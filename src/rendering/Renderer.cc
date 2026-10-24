#include "panlid/graphics/rendering/Renderer.hh"
#include "panlid/development/ThreadSafety.hh"
#include "panlid/graphics/BlendingMask.hh"
#include "panlid/graphics/BlendingParameters.hh"
#include "panlid/graphics/Colour4.hh"
#include "panlid/graphics/colour/ColourInfo.hh"
#include "panlid/graphics/primitives/RectangleF.hh"
#include "panlid/graphics/primitives/RectangleI.hh"
#include "panlid/graphics/rendering/ClearInfo.hh"
#include "panlid/graphics/rendering/DepthInfo.hh"
#include "panlid/graphics/rendering/FlushBatchSource.hh"
#include "panlid/graphics/rendering/GlobalUniformData.hh"
#include "panlid/graphics/rendering/INativeTexture.hh"
#include "panlid/graphics/rendering/IUniformBuffer.hh"
#include "panlid/graphics/rendering/IVertexBatch.hh"
#include "panlid/graphics/rendering/IVertexBuffer.hh"
#include "panlid/graphics/rendering/MaskingInfo.hh"
#include "panlid/graphics/rendering/PrimitiveTopology.hh"
#include "panlid/graphics/rendering/StencilInfo.hh"
#include "panlid/graphics/rendering/vertices/TexturedVertex2D.hh"
#include "panlid/graphics/shaders/IShader.hh"
#include "panlid/graphics/textures/WrapMode.hh"
#include "panlid/threading/ScheduledDelegate.hh"
#include "panlid/threading/Scheduler.hh"
#include "panlid/timing/StopwatchClock.hh"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace panlid::graphics::rendering {

namespace {

// Maps depth to [-1, 1]; shaders adapt via IsDepthRangeZeroToOne.
constexpr float kOrthoNear = -1.0F;
constexpr float kOrthoFar = 1.0F;

[[nodiscard]] glm::vec2 ToVec2(int x, int y) {
    return {static_cast<float>(x), static_cast<float>(y)};
}

[[nodiscard]] uint32_t ToUniformBool(bool value) { return value ? 1U : 0U; }

[[nodiscard]] glm::vec4 ToVec4(const Colour4& colour) {
    return {colour.R, colour.G, colour.B, colour.A};
}

// std140 stores each column of a mat3 in a padded vec4.
[[nodiscard]] std::array<glm::vec4, 3> ToStd140(const glm::mat3& matrix) {
    return {{glm::vec4{matrix[0], 0.0F}, glm::vec4{matrix[1], 0.0F}, glm::vec4{matrix[2], 0.0F}}};
}

// One colour per column (top-left, bottom-left, top-right, bottom-right), as the shader reads it.
[[nodiscard]] glm::mat4 ToBorderColourMatrix(const ColourInfo& colour) {
    return {ToVec4(colour.TopLeft.Srgb), ToVec4(colour.BottomLeft.Srgb),
            ToVec4(colour.TopRight.Srgb), ToVec4(colour.BottomRight.Srgb)};
}

// Pops the top of a state stack and returns the new top. The bottom entry can never be popped.
// Throws instead of asserting so the check also holds in release builds.
template <typename T> [[nodiscard]] T PopAndPeek(std::vector<T>& stack, std::string_view name) {
    if (stack.size() <= 1) {
        throw std::runtime_error(std::format("Cannot pop the last {} from the stack.", name));
    }

    stack.pop_back();
    return stack.back();
}

} // namespace

Renderer::Renderer()
    : disposalQueue_{kMaxDrawNodes},
      // Force no thread until we are actually on the draw thread.
      resetScheduler_{std::make_shared<threading::Scheduler>(
          [] { return development::ThreadSafety::IsDrawThread; },
          std::make_shared<timing::StopwatchClock>(true))} {
    // todo: register the renderer statistics (texture upload queue length, VBOs in use, flushes per
    // source). todo: create the white pixel texture lazily from a TextureAtlas once textures are
    // ported.
}

void Renderer::Initialise(platform::IGraphicsSurface& graphicsSurface) {
    // todo: assert the surface type matches the backend (OpenGL / Metal) once IGraphicsSurface is
    // ported.
    InitialiseImplementation(graphicsSurface);

    defaultQuadBatch_ = CreateQuadBatch<vertices::TexturedVertex2D>(100, 1000);
    resetScheduler_->AddDelayed([this] { disposalQueue_.CheckPendingDisposals(); }, 0, true);

    isInitialised_ = true;
}

void Renderer::BeginFrame(const glm::vec2& windowSize) {
    // todo: reset the per-source flush statistics.

    if (globalUniformBuffer_ == nullptr) {
        globalUniformBuffer_ = CreateUniformBuffer<GlobalUniformData>();
    }

    assert(defaultQuadBatch_ != nullptr);

    ++frameIndex_;
    backbufferDepth_.Reset();
    resetScheduler_->Update();

    runOneExpensiveOperation();
    resetFrameState();
    pushInitialState(windowSize);

    Clear(ClearInfo{.Colour = Colour4::Black()});

    freeUnusedVertexBuffers();
    uploadQueuedTextures();

    lastBoundTexture_.fill(nullptr);
    lastBoundTextureIsAtlas_.fill(false);
}

void Renderer::FinishFrame() { FlushCurrentBatch(FlushBatchSource::FinishFrame); }

void Renderer::ScheduleExpensiveOperation(std::shared_ptr<threading::ScheduledDelegate> operation) {
    if (!isInitialised_) {
        return;
    }

    std::scoped_lock lock{expensiveOperationLock_};
    expensiveOperations_.push_back(std::move(operation));
}

void Renderer::ScheduleDisposal(std::function<void()> disposalAction) {
    if (isInitialised_) {
        disposalQueue_.ScheduleDisposal(std::move(disposalAction));
    } else {
        disposalAction();
    }
}

void Renderer::runOneExpensiveOperation() {
    // todo: expose the queue length in the statistics.
    while (const auto operation = dequeueExpensiveOperation()) {
        if (operation->GetState() == threading::ScheduledDelegate::RunState::Waiting) {
            operation->RunTask();
            break;
        }
    }
}

std::shared_ptr<threading::ScheduledDelegate> Renderer::dequeueExpensiveOperation() {
    std::scoped_lock lock{expensiveOperationLock_};

    if (expensiveOperations_.empty()) {
        return nullptr;
    }

    auto operation = std::move(expensiveOperations_.front());
    expensiveOperations_.pop_front();
    return operation;
}

void Renderer::resetFrameState() {
    globalUniformsChanged_ = true;
    currentActiveBatch_ = nullptr;
    currentBlendingParameters_ = BlendingParameters{};
    currentMaskingInfo_ = MaskingInfo{};

    for (IVertexBatch* batch : batchResetList_) {
        batch->ResetCounters();
    }
    batchResetList_.clear();

    if (shader_ != nullptr) {
        shader_->Unbind();
    }
    shader_ = nullptr;

    boundUniformBuffers_.clear();
    viewportStack_.clear();
    projectionMatrixStack_.clear();
    maskingStack_.clear();
    scissorRectStack_.clear();
    frameBufferStack_.clear();
    depthStack_.clear();
    stencilStack_.clear();
    scissorStateStack_.clear();
    scissorOffsetStack_.clear();
    shaderStack_.clear();

    quadBatches_.clear();
    quadBatches_.push_back(defaultQuadBatch_.get());

    setFrameBuffer(nullptr, true);

    scissor_ = primitives::RectangleI{};
    scissorOffset_ = glm::ivec2{0};
    viewport_ = primitives::RectangleI{};
    projectionMatrix_ = glm::mat4{1.0F};
}

void Renderer::pushInitialState(const glm::vec2& windowSize) {
    const primitives::RectangleI window{.X = 0,
                                        .Y = 0,
                                        .Width = static_cast<int>(windowSize.x),
                                        .Height = static_cast<int>(windowSize.y)};

    PushScissorState(true);
    PushViewport(window);
    PushScissor(window);
    PushScissorOffset(glm::ivec2{0});
    PushMaskingInfo(
        {
            .ScreenSpaceAABB = window,
            .MaskingRect =
                primitives::RectangleF{
                    .X = 0.0F, .Y = 0.0F, .Width = windowSize.x, .Height = windowSize.y},
            .ToMaskingSpace = glm::mat3{1.0F},
            .CornerExponent = 2.5F,
            .BlendRange = 1.0F,
            .AlphaExponent = 1.0F,
        },
        true);

    PushDepthInfo(DepthInfo::Default());
    PushStencilInfo(StencilInfo::Default());
}

void Renderer::uploadQueuedTextures() {
    int queueLength = 0;
    {
        std::scoped_lock lock{textureUploadLock_};
        queueLength = static_cast<int>(textureUploadQueue_.size());
    }

    // todo: expose the upload queue length and the dequeued/performed counts in the statistics.

    // Increase the number of items processed with the queue length to ensure it doesn't get out of
    // hand.
    const int targetUploads = std::clamp(queueLength / 2, 1, GetMaxTexturesUploadedPerFrame());
    int uploads = 0;
    int uploadedPixels = 0;

    // Continue attempting to upload textures until enough uploads have been performed.
    while (INativeTexture* texture = dequeueTextureUpload()) {
        if (!texture->Upload()) {
            continue;
        }

        if (++uploads >= targetUploads) {
            break;
        }

        uploadedPixels += texture->GetWidth() * texture->GetHeight();
        if (uploadedPixels > GetMaxPixelsUploadedPerFrame()) {
            break;
        }
    }
}

INativeTexture* Renderer::dequeueTextureUpload() {
    std::scoped_lock lock{textureUploadLock_};

    if (textureUploadQueue_.empty()) {
        return nullptr;
    }

    INativeTexture* texture = textureUploadQueue_.front();
    textureUploadQueue_.pop_front();
    return texture;
}

void Renderer::Clear(const ClearInfo& clearInfo) {
    PushDepthInfo(DepthInfo{.WriteDepth = true});
    PushScissorState(false);

    ClearImplementation(clearInfo);
    currentClearInfo_ = clearInfo;

    PopScissorState();
    PopDepthInfo();
}

void Renderer::SetBlend(const BlendingParameters& blendingParameters) {
    if (currentBlendingParameters_ == blendingParameters) {
        return;
    }

    FlushCurrentBatch(FlushBatchSource::SetBlend);
    SetBlendImplementation(blendingParameters);

    currentBlendingParameters_ = blendingParameters;
}

void Renderer::SetBlendMask(BlendingMask blendingMask) {
    if (currentBlendingMask_ == blendingMask) {
        return;
    }

    FlushCurrentBatch(FlushBatchSource::SetBlendMask);
    SetBlendMaskImplementation(blendingMask);

    currentBlendingMask_ = blendingMask;
}

void Renderer::PushViewport(const primitives::RectangleI& viewport) {
    primitives::RectangleI actualRect = viewport;

    if (actualRect.Width < 0) {
        actualRect.X += viewport.Width;
        actualRect.Width = -viewport.Width;
    }

    if (actualRect.Height < 0) {
        actualRect.Y += viewport.Height;
        actualRect.Height = -viewport.Height;
    }

    pushOrtho(viewport);

    viewportStack_.push_back(actualRect);
    setViewport(viewport);
}

void Renderer::PopViewport() {
    PopProjectionMatrix();
    setViewport(PopAndPeek(viewportStack_, "viewport"));
}

void Renderer::pushOrtho(const primitives::RectangleI& area) {
    const auto left = static_cast<float>(area.X);
    const auto right = static_cast<float>(area.X + area.Width);
    const auto top = static_cast<float>(area.Y);
    const auto bottom = static_cast<float>(area.Y + area.Height);

    PushProjectionMatrix(glm::orthoRH_NO(left, right, bottom, top, kOrthoNear, kOrthoFar));
}

void Renderer::setViewport(const primitives::RectangleI& viewport) {
    if (viewport_ == viewport) {
        return;
    }

    SetViewportImplementation(viewport);
    viewport_ = viewport;
}

void Renderer::PushScissor(const primitives::RectangleI& scissor) {
    scissorRectStack_.push_back(scissor);
    setScissor(scissor);
}

void Renderer::PopScissor() { setScissor(PopAndPeek(scissorRectStack_, "scissor")); }

void Renderer::PushScissorState(bool enabled) {
    scissorStateStack_.push_back(enabled);
    setScissorState(enabled);
}

void Renderer::PopScissorState() {
    setScissorState(PopAndPeek(scissorStateStack_, "scissor state"));
}

void Renderer::PushScissorOffset(glm::ivec2 offset) {
    scissorOffsetStack_.push_back(offset);
    setScissorOffset(offset);
}

void Renderer::PopScissorOffset() {
    setScissorOffset(PopAndPeek(scissorOffsetStack_, "scissor offset"));
}

void Renderer::setScissor(primitives::RectangleI scissor) {
    if (scissor.Width < 0) {
        scissor.X += scissor.Width;
        scissor.Width = -scissor.Width;
    }

    if (scissor.Height < 0) {
        scissor.Y += scissor.Height;
        scissor.Height = -scissor.Height;
    }

    if (scissor_ == scissor) {
        return;
    }

    // On a backend whose UV origin is bottom-left, vertex positions are flipped vertically when
    // rendering to a framebuffer. The scissor box has to be flipped the same way for scissoring to
    // stay correct.
    primitives::RectangleI compensatedScissor = scissor;
    if (!usingBackbuffer_ && !IsUvOriginTopLeft()) {
        compensatedScissor.Y = viewport_.Height - (scissor.Y + scissor.Height);
    }

    FlushCurrentBatch(FlushBatchSource::SetScissor);
    SetScissorImplementation(compensatedScissor);

    // Do not expose the implementation detail of flipping the scissor box to readers of the
    // scissor.
    scissor_ = scissor;
}

void Renderer::setScissorState(bool enabled) {
    if (enabled == scissorEnabled_) {
        return;
    }

    FlushCurrentBatch(FlushBatchSource::SetScissor);
    SetScissorStateImplementation(enabled);
    scissorEnabled_ = enabled;
}

void Renderer::setScissorOffset(glm::ivec2 offset) {
    if (scissorOffset_ == offset) {
        return;
    }

    FlushCurrentBatch(FlushBatchSource::SetScissor);
    scissorOffset_ = offset;
}

void Renderer::PushProjectionMatrix(const glm::mat4& matrix) {
    projectionMatrixStack_.push_back(matrix);
    setProjectionMatrix(matrix);
}

void Renderer::PopProjectionMatrix() {
    setProjectionMatrix(PopAndPeek(projectionMatrixStack_, "projection matrix"));
}

void Renderer::setProjectionMatrix(const glm::mat4& matrix) {
    if (projectionMatrix_ == matrix) {
        return;
    }

    FlushCurrentBatch(FlushBatchSource::SetProjection);

    globalUniformsChanged_ = true;
    projectionMatrix_ = matrix;
}

void Renderer::PushMaskingInfo(const MaskingInfo& maskingInfo, bool overwritePreviousScissor) {
    maskingStack_.push_back(maskingInfo);
    setMaskingInfo(maskingInfo, true, overwritePreviousScissor);
}

void Renderer::PopMaskingInfo() {
    setMaskingInfo(PopAndPeek(maskingStack_, "masking info"), false, true);
}

void Renderer::setMaskingInfo(const MaskingInfo& maskingInfo, bool isPushing,
                              bool overwritePreviousScissor) {
    if (currentMaskingInfo_ == maskingInfo) {
        return;
    }

    FlushCurrentBatch(FlushBatchSource::SetMasking);

    if (isPushing) {
        // When drawing to a viewport that doesn't match the projection size (e.g. via
        // framebuffers), the resultant image will be scaled.
        const glm::vec2 projectionScale{projectionMatrix_[0][0] / 2.0F,
                                        -projectionMatrix_[1][1] / 2.0F};
        const glm::vec2 viewportScale = ToVec2(viewport_.Width, viewport_.Height) * projectionScale;

        const primitives::RectangleI& aabb = maskingInfo.ScreenSpaceAABB;
        const glm::vec2 location =
            ToVec2(aabb.X - scissorOffset_.x, aabb.Y - scissorOffset_.y) * viewportScale;
        const glm::vec2 size = ToVec2(aabb.Width, aabb.Height) * viewportScale;

        const primitives::RectangleI actualRect{
            .X = static_cast<int>(std::floor(location.x)),
            .Y = static_cast<int>(std::floor(location.y)),
            .Width = static_cast<int>(std::ceil(size.x)),
            .Height = static_cast<int>(std::ceil(size.y)),
        };

        PushScissor(overwritePreviousScissor
                        ? actualRect
                        : primitives::RectangleI::Intersect(scissorRectStack_.back(), actualRect));
    } else {
        PopScissor();
    }

    currentMaskingInfo_ = maskingInfo;

    // Masking is enabled for as long as any masking info is active that's not the default.
    isMaskingActive_ = maskingStack_.size() > 1;
    globalUniformsChanged_ = true;
}

void Renderer::PushDepthInfo(const DepthInfo& depthInfo) {
    depthStack_.push_back(depthInfo);
    setDepthInfo(depthInfo);
}

void Renderer::PopDepthInfo() { setDepthInfo(PopAndPeek(depthStack_, "depth info")); }

void Renderer::PushStencilInfo(const StencilInfo& stencilInfo) {
    stencilStack_.push_back(stencilInfo);
    setStencilInfo(stencilInfo);
}

void Renderer::PopStencilInfo() { setStencilInfo(PopAndPeek(stencilStack_, "stencil info")); }

void Renderer::setDepthInfo(const DepthInfo& depthInfo) {
    if (currentDepthInfo_ == depthInfo) {
        return;
    }

    FlushCurrentBatch(FlushBatchSource::SetDepthInfo);
    SetDepthInfoImplementation(depthInfo);

    currentDepthInfo_ = depthInfo;
}

void Renderer::setStencilInfo(const StencilInfo& stencilInfo) {
    if (currentStencilInfo_ == stencilInfo) {
        return;
    }

    FlushCurrentBatch(FlushBatchSource::SetStencilInfo);
    SetStencilInfoImplementation(stencilInfo);

    currentStencilInfo_ = stencilInfo;
}

namespace {

// A negative unit becomes a huge index, which at() rejects.
[[nodiscard]] std::size_t ToIndex(int unit) { return static_cast<std::size_t>(unit); }

} // namespace

void Renderer::PushQuadBatch(IVertexBatch& quadBatch) { quadBatches_.push_back(&quadBatch); }

void Renderer::PopQuadBatch() {
    assert(quadBatches_.size() > 1);
    quadBatches_.pop_back();
}

void Renderer::RegisterVertexBufferUse(const std::shared_ptr<IVertexBuffer>& buffer) {
    vertexBuffersInUse_.push_back(buffer);
}

void Renderer::SetActiveBatch(IVertexBatch& batch) {
    if (currentActiveBatch_ == &batch) {
        return;
    }

    batchResetList_.push_back(&batch);

    FlushCurrentBatch(FlushBatchSource::SetActiveBatch);

    currentActiveBatch_ = &batch;
}

void Renderer::FlushCurrentBatch([[maybe_unused]] std::optional<FlushBatchSource> source) {
    if (currentActiveBatch_ != nullptr) {
        // todo: when something was drawn and a source is given, count it in the per-source flush
        // statistics.
        currentActiveBatch_->Draw();
    }
}

void Renderer::freeUnusedVertexBuffers() {
    for (const std::weak_ptr<IVertexBuffer>& weakBuffer : vertexBuffersInUse_) {
        const std::shared_ptr<IVertexBuffer> buffer = weakBuffer.lock();

        if (buffer != nullptr && buffer->IsInUse() &&
            frameIndex_ - buffer->GetLastUseFrameIndex() > kResourceFreeNoUsageLength) {
            // Free() marks the buffer as no longer in use, which lets the cleanup below remove it.
            buffer->Free();
        }
    }

    std::erase_if(vertexBuffersInUse_, [](const std::weak_ptr<IVertexBuffer>& weakBuffer) {
        const std::shared_ptr<IVertexBuffer> buffer = weakBuffer.lock();
        return buffer == nullptr || !buffer->IsInUse();
    });

    // todo: expose the number of buffers in use in the statistics.
}

bool Renderer::BindTexture(INativeTexture& texture, int unit, textures::WrapMode wrapModeS,
                           textures::WrapMode wrapModeT) {
    if (lastActiveTextureUnit_ == unit && lastBoundTexture_.at(ToIndex(unit)) == &texture) {
        setWrapMode(wrapModeS, wrapModeT);
        return true;
    }

    FlushCurrentBatch(FlushBatchSource::BindTexture);

    if (!SetTextureImplementation(&texture, unit)) {
        return false;
    }

    setWrapMode(wrapModeS, wrapModeT);

    lastBoundTexture_.at(ToIndex(unit)) = &texture;
    lastBoundTextureIsAtlas_.at(ToIndex(unit)) = false;
    lastActiveTextureUnit_ = unit;

    // todo: count the bind in the frame statistics (TextureBinds).
    texture.SetTotalBindCount(texture.GetTotalBindCount() + 1);

    return true;
}

void Renderer::setWrapMode(textures::WrapMode wrapModeS, textures::WrapMode wrapModeT) {
    if (wrapModeS != currentWrapModeS_) {
        FlushCurrentBatch(FlushBatchSource::BindTexture);

        currentWrapModeS_ = wrapModeS;
        globalUniformsChanged_ = true;
    }

    if (wrapModeT != currentWrapModeT_) {
        FlushCurrentBatch(FlushBatchSource::BindTexture);

        currentWrapModeT_ = wrapModeT;
        globalUniformsChanged_ = true;
    }
}

void Renderer::UnbindTexture(int unit) {
    if (lastBoundTexture_.at(ToIndex(unit)) == nullptr) {
        return;
    }

    FlushCurrentBatch(FlushBatchSource::UnbindTexture);
    SetTextureImplementation(nullptr, unit);

    lastBoundTexture_.at(ToIndex(unit)) = nullptr;
    lastBoundTextureIsAtlas_.at(ToIndex(unit)) = false;
}

void Renderer::EnqueueTextureUpload(INativeTexture& texture) {
    if (!isInitialised_) {
        return;
    }

    std::scoped_lock lock{textureUploadLock_};

    if (std::ranges::find(textureUploadQueue_, &texture) != textureUploadQueue_.end()) {
        return;
    }

    textureUploadQueue_.push_back(&texture);
}

void Renderer::BindFrameBuffer(IFrameBuffer& frameBuffer) {
    frameBufferStack_.push_back(&frameBuffer);
    setFrameBuffer(&frameBuffer);
}

void Renderer::UnbindFrameBuffer(IFrameBuffer* frameBuffer) {
    if (frameBuffer_ != frameBuffer) {
        return;
    }

    if (frameBufferStack_.empty()) {
        throw std::runtime_error("No frame buffer to unbind.");
    }

    frameBufferStack_.pop_back();
    setFrameBuffer(frameBufferStack_.empty() ? nullptr : frameBufferStack_.back());
}

void Renderer::setFrameBuffer(IFrameBuffer* frameBuffer, bool force) {
    if (frameBuffer == frameBuffer_ && !force) {
        return;
    }

    FlushCurrentBatch(FlushBatchSource::SetFrameBuffer);
    SetFrameBufferImplementation(frameBuffer);

    frameBuffer_ = frameBuffer;

    usingBackbuffer_ = frameBuffer == nullptr;
    globalUniformsChanged_ = true;
}

void Renderer::DeleteFrameBuffer(std::shared_ptr<IFrameBuffer> frameBuffer) {
    while (frameBuffer_ == frameBuffer.get()) {
        UnbindFrameBuffer(frameBuffer.get());
    }

    ScheduleDisposal([this, frameBuffer = std::move(frameBuffer)] {
        DeleteFrameBufferImplementation(*frameBuffer);
    });
}

void Renderer::DrawVertices(PrimitiveTopology topology, int vertexStart, int verticesCount) {
    if (shader_ == nullptr) {
        throw std::runtime_error("No shader bound.");
    }

    assert(globalUniformBuffer_ != nullptr);

    if (globalUniformsChanged_) {
        updateGlobalUniforms();
        globalUniformsChanged_ = false;
    }

    shader_->BindUniformBlock("g_GlobalUniforms", *globalUniformBuffer_);

    DrawVerticesImplementation(topology, vertexStart, verticesCount);

    // todo: count the draw call and the drawn vertices in the frame statistics.
}

void Renderer::updateGlobalUniforms() {
    const MaskingInfo& masking = currentMaskingInfo_;
    const primitives::RectangleF& maskingRect = masking.MaskingRect;

    // Updated in place: BorderColour and InnerCornerRadius keep their previous value when they
    // don't apply.
    GlobalUniformData& data = globalUniformData_;

    data.BackbufferDraw = ToUniformBool(usingBackbuffer_);
    data.IsDepthRangeZeroToOne = ToUniformBool(IsDepthRangeZeroToOne());
    data.IsClipSpaceYInverted = ToUniformBool(IsClipSpaceYInverted());
    data.IsUvOriginTopLeft = ToUniformBool(IsUvOriginTopLeft());
    data.ProjMatrix = projectionMatrix_;
    data.ToMaskingSpace = ToStd140(masking.ToMaskingSpace);
    data.IsMasking = ToUniformBool(isMaskingActive_);
    data.CornerRadius = masking.CornerRadius;
    data.CornerExponent = masking.CornerExponent;
    data.MaskingRect = {maskingRect.X, maskingRect.Y, maskingRect.X + maskingRect.Width,
                        maskingRect.Y + maskingRect.Height};
    data.BorderThickness = masking.BorderThickness / masking.BlendRange;

    if (masking.BorderThickness > 0) {
        data.BorderColour = ToBorderColourMatrix(masking.BorderColour);
    }

    data.MaskingBlendRange = masking.BlendRange;
    data.AlphaExponent = masking.AlphaExponent;
    data.EdgeOffset = masking.EdgeOffset;
    data.DiscardInner = ToUniformBool(masking.Hollow);

    if (masking.Hollow) {
        data.InnerCornerRadius = masking.HollowCornerRadius;
    }

    data.WrapModeS = static_cast<std::int32_t>(currentWrapModeS_);
    data.WrapModeT = static_cast<std::int32_t>(currentWrapModeT_);

    globalUniformBuffer_->SetData(std::as_bytes(std::span{&data, 1}));
}

void Renderer::BindShader(shaders::IShader& shader) {
    const bool alreadyBound = !shaderStack_.empty() && shaderStack_.back() == &shader;

    shaderStack_.push_back(&shader);

    if (!alreadyBound) {
        setShader(&shader);
    }
}

void Renderer::UnbindShader(shaders::IShader& shader) {
    if (shaderStack_.empty() || shaderStack_.back() != &shader) {
        throw std::runtime_error(
            "Attempting to unbind a shader that isn't the latest bound shader.");
    }

    shaderStack_.pop_back();
    setShader(shaderStack_.empty() ? nullptr : shaderStack_.back());
}

void Renderer::setShader(shaders::IShader* shader) {
    development::ThreadSafety::EnsureDrawThread();

    if (shader == shader_ || shader == nullptr) {
        return;
    }

    // todo: count the shader bind in the frame statistics.
    FlushCurrentBatch(FlushBatchSource::SetShader);
    SetShaderImplementation(*shader);

    // Importantly, when a shader is unbound it remains bound in the implementation.
    // To save VBO flushing overhead, keep a reference to the last shader.
    shader_ = shader;
}

void Renderer::BindUniformBuffer(std::string_view blockName, IUniformBuffer& buffer) {
    const auto current = boundUniformBuffers_.find(blockName);
    if (current != boundUniformBuffers_.end() && current->second == &buffer) {
        return;
    }

    FlushCurrentBatch(FlushBatchSource::BindBuffer);
    SetUniformBufferImplementation(blockName, buffer);

    boundUniformBuffers_[std::string{blockName}] = &buffer;
}

void Renderer::validateMaxBuffers(int maxBuffers) {
    if (maxBuffers <= 0) {
        throw std::invalid_argument("Maximum number of buffers must be > 0.");
    }
}

void Renderer::validateLinearBatch(int size, int maxBuffers) {
    if (size <= 0) {
        throw std::invalid_argument("Linear batch size must be > 0.");
    }

    if (size > kMaxVertices) {
        throw std::invalid_argument(
            std::format("Linear batch may not have more than {} vertices.", kMaxVertices));
    }

    validateMaxBuffers(maxBuffers);
}

void Renderer::validateQuadBatch(int size, int maxBuffers) {
    if (size <= 0) {
        throw std::invalid_argument("Quad batch size must be > 0.");
    }

    if (size > kMaxQuads) {
        throw std::invalid_argument(
            std::format("Quad batch may not have more than {} quads.", kMaxQuads));
    }

    validateMaxBuffers(maxBuffers);
}

} // namespace panlid::graphics::rendering
