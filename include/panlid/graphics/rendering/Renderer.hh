#pragma once

#include "panlid/graphics/BlendingMask.hh"
#include "panlid/graphics/BlendingParameters.hh"
#include "panlid/graphics/Colour4.hh"
#include "panlid/graphics/primitives/RectangleI.hh"
#include "panlid/graphics/rendering/BufferTestFunction.hh"
#include "panlid/graphics/rendering/ClearInfo.hh"
#include "panlid/graphics/rendering/DepthInfo.hh"
#include "panlid/graphics/rendering/DepthValue.hh"
#include "panlid/graphics/rendering/FlushBatchSource.hh"
#include "panlid/graphics/rendering/GlobalUniformData.hh"
#include "panlid/graphics/rendering/IFrameBuffer.hh"
#include "panlid/graphics/rendering/INativeTexture.hh"
#include "panlid/graphics/rendering/IUniformBuffer.hh"
#include "panlid/graphics/rendering/IVertexBatch.hh"
#include "panlid/graphics/rendering/IVertexBuffer.hh"
#include "panlid/graphics/rendering/MaskingInfo.hh"
#include "panlid/graphics/rendering/PrimitiveTopology.hh"
#include "panlid/graphics/rendering/RendererDisposalQueue.hh"
#include "panlid/graphics/rendering/StencilInfo.hh"
#include "panlid/graphics/rendering/StencilOperation.hh"
#include "panlid/graphics/rendering/vertices/Vertex.hh"
#include "panlid/graphics/rendering/vertices/VertexMember.hh"
#include "panlid/graphics/shaders/IShader.hh"
#include "panlid/graphics/textures/TextureFilteringMode.hh"
#include "panlid/graphics/textures/WrapMode.hh"
#include "panlid/platform/IGraphicsSurface.hh"
#include "panlid/threading/ScheduledDelegate.hh"
#include "panlid/threading/Scheduler.hh"
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace panlid::platform {

class GameHost;

} // namespace panlid::platform

namespace panlid::graphics::rendering {

// Draws to the screen. Backends derive from this class and implement the *Implementation hooks.
//
// The renderer keeps non-owning pointers to batches, shaders, framebuffers, textures and uniform
// buffers. Whoever created one keeps it alive for as long as the renderer can still reach it:
// until it is popped or unbound, and for a batch until the end of the frame it was last active in.
class Renderer {
  public:
    // Maximum number of DrawNodes a Drawable draws with. Chosen to let the update and draw threads
    // run concurrently without unnecessary load.
    static constexpr int kMaxDrawNodes = 3;
    static constexpr int kMaxMipmapLevels = 3;
    static constexpr int kVerticesPerQuad = 4;
    static constexpr int kIndicesPerQuad = kVerticesPerQuad + 2;
    // Maximum number of vertices in a linear vertex buffer.
    static constexpr int kMaxVertices = 65535;
    static constexpr int kMaxQuads = kMaxVertices / kIndicesPerQuad;
    // Frames without use before an unused resource is freed.
    static constexpr uint64_t kResourceFreeNoUsageLength = 300;

    Renderer();
    Renderer(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer& operator=(Renderer&&) = delete;
    virtual ~Renderer() = default;

    // todo: shader compilation cache storage, once Storage and ShaderCompilationStore exist.

    [[nodiscard]] uint64_t GetFrameIndex() const { return frameIndex_; }

    [[nodiscard]] int GetMaxTextureSize() const { return maxTextureSize_; }

    // Maximum number of queued textures uploaded per frame.
    [[nodiscard]] int GetMaxTexturesUploadedPerFrame() const {
        return maxTexturesUploadedPerFrame_;
    }
    void SetMaxTexturesUploadedPerFrame(int count) { maxTexturesUploadedPerFrame_ = count; }

    // Maximum number of pixels uploaded per frame. Defaults to 2 megapixels (8 MB).
    [[nodiscard]] int GetMaxPixelsUploadedPerFrame() const { return maxPixelsUploadedPerFrame_; }
    void SetMaxPixelsUploadedPerFrame(int count) { maxPixelsUploadedPerFrame_ = count; }

    // Whether depth is in [0, 1]. If false, depth is in [-1, 1].
    [[nodiscard]] virtual bool IsDepthRangeZeroToOne() const = 0;

    // Whether texture coordinates begin at the top-left of a texture. If false, (0, 0) is the
    // bottom-left texel.
    [[nodiscard]] virtual bool IsUvOriginTopLeft() const = 0;

    // Whether clip-space y runs from -1 (top) to 1 (bottom). If false, it runs from -1 (bottom) to
    // 1 (top).
    [[nodiscard]] virtual bool IsClipSpaceYInverted() const = 0;

    [[nodiscard]] const MaskingInfo& GetCurrentMaskingInfo() const { return currentMaskingInfo_; }
    [[nodiscard]] primitives::RectangleI GetViewport() const { return viewport_; }
    [[nodiscard]] primitives::RectangleI GetScissor() const { return scissor_; }
    [[nodiscard]] glm::ivec2 GetScissorOffset() const { return scissorOffset_; }
    [[nodiscard]] const glm::mat4& GetProjectionMatrix() const { return projectionMatrix_; }
    [[nodiscard]] DepthInfo GetCurrentDepthInfo() const { return currentDepthInfo_; }
    [[nodiscard]] StencilInfo GetCurrentStencilInfo() const { return currentStencilInfo_; }
    [[nodiscard]] textures::WrapMode GetCurrentWrapModeS() const { return currentWrapModeS_; }
    [[nodiscard]] textures::WrapMode GetCurrentWrapModeT() const { return currentWrapModeT_; }
    [[nodiscard]] bool IsMaskingActive() const { return isMaskingActive_; }
    [[nodiscard]] bool IsUsingBackbuffer() const { return usingBackbuffer_; }
    // todo: white pixel texture, once Texture and TextureAtlas exist.

    // Depth of textured vertices when drawn to the backbuffer.
    // todo: DrawNodes also change this; decide the mutation API when DrawNode is ported.
    [[nodiscard]] const DepthValue& GetBackbufferDepth() const { return backbufferDepth_; }

    [[nodiscard]] bool IsInitialised() const { return isInitialised_; }
    [[nodiscard]] BlendingParameters GetCurrentBlendingParameters() const {
        return currentBlendingParameters_;
    }

    // Queues an operation. At most one queued operation runs per frame.
    void ScheduleExpensiveOperation(std::shared_ptr<threading::ScheduledDelegate> operation);

    // Runs the action a few frames from now, or immediately if not yet initialised. The action must
    // own everything it touches: capture shared_ptrs or moved-in objects, never raw pointers or
    // references to objects that may be destroyed before it runs.
    void ScheduleDisposal(std::function<void()> disposalAction);

    // todo: TakeScreenshot and ExtractFrameBufferData, once an image library is chosen.

    // Clears the currently bound framebuffer.
    void Clear(const ClearInfo& clearInfo);

    void SetBlend(const BlendingParameters& blendingParameters);

    // Sets which colour components blending affects.
    void SetBlendMask(BlendingMask blendingMask);

    void PushViewport(const primitives::RectangleI& viewport);
    void PopViewport();

    void PushScissor(const primitives::RectangleI& scissor);
    void PopScissor();

    void PushScissorState(bool enabled);
    void PopScissorState();

    void PushScissorOffset(glm::ivec2 offset);
    void PopScissorOffset();

    void PushProjectionMatrix(const glm::mat4& matrix);
    void PopProjectionMatrix();

    void PushMaskingInfo(const MaskingInfo& maskingInfo, bool overwritePreviousScissor = false);
    void PopMaskingInfo();

    void PushDepthInfo(const DepthInfo& depthInfo);
    void PopDepthInfo();

    void PushStencilInfo(const StencilInfo& stencilInfo);
    void PopStencilInfo();

    [[nodiscard]] IVertexBatch& GetDefaultQuadBatch() { return *quadBatches_.back(); }
    void PushQuadBatch(IVertexBatch& quadBatch);
    void PopQuadBatch();

    // Marks a vertex buffer as in use, so it is freed after going unused for a while.
    void RegisterVertexBufferUse(const std::shared_ptr<IVertexBuffer>& buffer);

    // Records the batch that drew last, so a state change can flush it first.
    void SetActiveBatch(IVertexBatch& batch);

    // Draws the active batch. The source is for profiling.
    void FlushCurrentBatch(std::optional<FlushBatchSource> source);

    // Returns whether the texture was bound.
    // todo: overload taking a Texture with optional wrap modes, once Texture exists.
    bool BindTexture(INativeTexture& texture, int unit = 0,
                     textures::WrapMode wrapModeS = textures::WrapMode::None,
                     textures::WrapMode wrapModeT = textures::WrapMode::None);

    void UnbindTexture(int unit = 0);

    // Queues a texture for upload next frame. The texture must stay alive while queued.
    // todo: a texture should remove itself from the queue when destroyed.
    void EnqueueTextureUpload(INativeTexture& texture);

    void BindFrameBuffer(IFrameBuffer& frameBuffer);
    void UnbindFrameBuffer(IFrameBuffer* frameBuffer);

    // The renderer keeps the framebuffer alive until the delayed deletion has run.
    void DeleteFrameBuffer(std::shared_ptr<IFrameBuffer> frameBuffer);

    void DrawVertices(PrimitiveTopology topology, int vertexStart, int verticesCount);

    void BindShader(shaders::IShader& shader);
    void UnbindShader(shaders::IShader& shader);
    // todo: SetUniform, once IUniform and IUniformWithValue exist.

    void BindUniformBuffer(std::string_view blockName, IUniformBuffer& buffer);

    // todo: shader part and shader creation, mipmap shader, framebuffer, texture and video texture
    // creation, shader storage buffers.

    // Creates a batch drawing the vertices as the given primitive type. size is the number of
    // vertices per buffer.
    template <vertices::Vertex T>
    [[nodiscard]] std::unique_ptr<IVertexBatch> CreateLinearBatch(int size, int maxBuffers,
                                                                  PrimitiveTopology topology) {
        validateLinearBatch(size, maxBuffers);
        const auto layout = T::GetLayout();
        return CreateLinearBatchImplementation(layout, sizeof(T), size, maxBuffers, topology);
    }

    // Creates a batch drawing the vertices as quads. size is the number of quads per buffer.
    template <vertices::Vertex T>
    [[nodiscard]] std::unique_ptr<IVertexBatch> CreateQuadBatch(int size, int maxBuffers) {
        validateQuadBatch(size, maxBuffers);
        const auto layout = T::GetLayout();
        return CreateQuadBatchImplementation(layout, sizeof(T), size, maxBuffers);
    }

    // Creates a buffer holding the data of a shader's uniform block.
    template <typename TData>
        requires std::is_trivially_copyable_v<TData>
    [[nodiscard]] std::unique_ptr<IUniformBuffer> CreateUniformBuffer() {
        static_assert(sizeof(TData) % kUniformBlockAlignment == 0,
                      "uniform block data must be padded to a multiple of 16 bytes");
        return CreateUniformBufferImplementation(sizeof(TData));
    }

  protected:
    // Frame lifecycle, driven by GameHost.

    [[nodiscard]] virtual bool GetVerticalSync() const = 0;
    virtual void SetVerticalSync(bool verticalSync) = 0;

    [[nodiscard]] virtual bool GetAllowTearing() const = 0;
    virtual void SetAllowTearing(bool allowTearing) = 0;

    // One-off initialisation.
    void Initialise(platform::IGraphicsSurface& graphicsSurface);

    // Resets state to prepare for a new frame. windowSize is the full window size.
    virtual void BeginFrame(const glm::vec2& windowSize);

    // Performs any last actions before the frame ends.
    virtual void FinishFrame();

    // Presents the finished frame.
    virtual void SwapBuffers() = 0;

    // Blocks until the GPU has finished all submitted commands.
    virtual void WaitUntilIdle() = 0;

    // Blocks until the GPU is ready for the next frame.
    virtual void WaitUntilNextFrameReady() = 0;

    // Called when the draw thread becomes active and will submit commands.
    virtual void MakeCurrent() = 0;

    // Called when the draw thread is suspended and will submit no more commands.
    virtual void ClearCurrent() = 0;

    // For backends.

    void SetMaxTextureSize(int size) { maxTextureSize_ = size; }

    [[nodiscard]] bool IsScissorEnabled() const { return scissorEnabled_; }
    [[nodiscard]] ClearInfo GetCurrentClearInfo() const { return currentClearInfo_; }
    [[nodiscard]] BlendingMask GetCurrentBlendingMask() const { return currentBlendingMask_; }

    // The bound framebuffer, or null for the backbuffer.
    [[nodiscard]] IFrameBuffer* GetFrameBuffer() const { return frameBuffer_; }

    // The bound shader, or null if none.
    [[nodiscard]] shaders::IShader* GetShader() const { return shader_; }

    virtual void InitialiseImplementation(platform::IGraphicsSurface& graphicsSurface) = 0;

    // Clears the colour and depth targets of the bound framebuffer.
    virtual void ClearImplementation(const ClearInfo& clearInfo) = 0;

    virtual void SetBlendImplementation(const BlendingParameters& blendingParameters) = 0;
    virtual void SetBlendMaskImplementation(BlendingMask blendingMask) = 0;
    virtual void SetViewportImplementation(const primitives::RectangleI& viewport) = 0;
    virtual void SetScissorImplementation(const primitives::RectangleI& scissor) = 0;
    virtual void SetScissorStateImplementation(bool enabled) = 0;
    virtual void SetDepthInfoImplementation(const DepthInfo& depthInfo) = 0;
    virtual void SetStencilInfoImplementation(const StencilInfo& stencilInfo) = 0;

    // A null texture means the default texture. Returns whether the texture was set.
    virtual bool SetTextureImplementation(INativeTexture* texture, int unit) = 0;

    // A null framebuffer means the backbuffer.
    virtual void SetFrameBufferImplementation(IFrameBuffer* frameBuffer) = 0;

    // Runs frames after the framebuffer was released.
    // todo: a backend framebuffer should keep its GPU handles in a separately owned object; decide
    // with the first real backend.
    virtual void DeleteFrameBufferImplementation(IFrameBuffer& frameBuffer) = 0;

    virtual void DrawVerticesImplementation(PrimitiveTopology topology, int vertexStart,
                                            int verticesCount) = 0;
    virtual void SetShaderImplementation(shaders::IShader& shader) = 0;
    virtual void SetUniformBufferImplementation(std::string_view blockName,
                                                IUniformBuffer& buffer) = 0;

    // The layout span is only valid during the call; backends must copy it.
    virtual std::unique_ptr<IVertexBatch>
    CreateLinearBatchImplementation(std::span<const vertices::VertexMember> layout,
                                    std::size_t vertexSize, int size, int maxBuffers,
                                    PrimitiveTopology topology) = 0;
    virtual std::unique_ptr<IVertexBatch>
    CreateQuadBatchImplementation(std::span<const vertices::VertexMember> layout,
                                  std::size_t vertexSize, int size, int maxBuffers) = 0;
    virtual std::unique_ptr<IUniformBuffer> CreateUniformBufferImplementation(std::size_t size) = 0;

    virtual std::unique_ptr<INativeTexture>
    CreateNativeTexture(int width, int height, bool manualMipmaps,
                        textures::TextureFilteringMode filteringMode,
                        const std::optional<Colour4>& initialisationColour) = 0;
    virtual std::unique_ptr<INativeTexture> CreateNativeVideoTexture(int width, int height) = 0;

  private:
    friend class platform::GameHost;

    static constexpr int kMaxTextureUnits = 16;
    static constexpr std::size_t kUniformBlockAlignment = 16;

    // Read from other threads.
    std::atomic<bool> isInitialised_ = false;
    std::atomic<uint64_t> frameIndex_ = 0;
    // The default lets headless runs without a graphics context behave normally.
    std::atomic<int> maxTextureSize_ = 4096;
    std::atomic<int> maxTexturesUploadedPerFrame_ = 32;
    std::atomic<int> maxPixelsUploadedPerFrame_ = 1024 * 1024 * 2;

    // State starts zeroed instead of at the usual defaults, so the first frame's pushes always
    // differ from it and reach the backend.
    primitives::RectangleI viewport_;
    primitives::RectangleI scissor_;
    glm::ivec2 scissorOffset_{};
    glm::mat4 projectionMatrix_{1.0F};
    DepthInfo currentDepthInfo_{
        .DepthTest = false, .WriteDepth = false, .Function = BufferTestFunction::Never};
    StencilInfo currentStencilInfo_{.StencilTest = false,
                                    .TestFunction = BufferTestFunction::Never,
                                    .TestValue = 0,
                                    .Mask = 0,
                                    .StencilTestFailOperation = StencilOperation::Zero,
                                    .DepthTestFailOperation = StencilOperation::Zero,
                                    .TestPassedOperation = StencilOperation::Zero};
    textures::WrapMode currentWrapModeS_{};
    textures::WrapMode currentWrapModeT_{};
    bool isMaskingActive_ = false;
    bool usingBackbuffer_ = false;
    ClearInfo currentClearInfo_;
    BlendingParameters currentBlendingParameters_;
    BlendingMask currentBlendingMask_{};
    bool scissorEnabled_ = false;
    MaskingInfo currentMaskingInfo_;
    IFrameBuffer* frameBuffer_ = nullptr;
    shaders::IShader* shader_ = nullptr;

    int lastActiveTextureUnit_ = 0;
    bool globalUniformsChanged_ = false;
    GlobalUniformData globalUniformData_;
    std::unique_ptr<IUniformBuffer> globalUniformBuffer_;
    std::unique_ptr<IVertexBatch> defaultQuadBatch_;
    IVertexBatch* currentActiveBatch_ = nullptr;

    // Declaration order matters: the constructor initialises these in this order.
    RendererDisposalQueue disposalQueue_;
    std::shared_ptr<threading::Scheduler> resetScheduler_;
    DepthValue backbufferDepth_;

    std::map<std::string, IUniformBuffer*, std::less<>> boundUniformBuffers_;
    std::vector<IVertexBatch*> quadBatches_;
    std::vector<std::weak_ptr<IVertexBuffer>> vertexBuffersInUse_;
    std::vector<IVertexBatch*> batchResetList_;
    std::vector<primitives::RectangleI> viewportStack_;
    std::vector<glm::mat4> projectionMatrixStack_;
    std::vector<MaskingInfo> maskingStack_;
    std::vector<primitives::RectangleI> scissorRectStack_;
    std::vector<DepthInfo> depthStack_;
    std::vector<StencilInfo> stencilStack_;
    std::vector<glm::ivec2> scissorOffsetStack_;
    std::vector<IFrameBuffer*> frameBufferStack_;
    std::vector<shaders::IShader*> shaderStack_;
    std::vector<bool> scissorStateStack_;

    std::array<INativeTexture*, kMaxTextureUnits> lastBoundTexture_{};
    std::array<bool, kMaxTextureUnits> lastBoundTextureIsAtlas_{};

    std::mutex expensiveOperationLock_;
    std::deque<std::shared_ptr<threading::ScheduledDelegate>> expensiveOperations_;
    std::mutex textureUploadLock_;
    std::deque<INativeTexture*> textureUploadQueue_;

    static void validateLinearBatch(int size, int maxBuffers);
    static void validateQuadBatch(int size, int maxBuffers);
    static void validateMaxBuffers(int maxBuffers);

    void runOneExpensiveOperation();
    void resetFrameState();
    void pushInitialState(const glm::vec2& windowSize);
    void uploadQueuedTextures();
    void freeUnusedVertexBuffers();
    void updateGlobalUniforms();
    [[nodiscard]] INativeTexture* dequeueTextureUpload();
    [[nodiscard]] std::shared_ptr<threading::ScheduledDelegate> dequeueExpensiveOperation();

    // todo: replace with a shared PushOrtho helper once the projection helpers are ported.
    void pushOrtho(const primitives::RectangleI& area);
    void setViewport(const primitives::RectangleI& viewport);
    void setScissor(primitives::RectangleI scissor);
    void setScissorState(bool enabled);
    void setScissorOffset(glm::ivec2 offset);
    void setProjectionMatrix(const glm::mat4& matrix);
    void setMaskingInfo(const MaskingInfo& maskingInfo, bool isPushing,
                        bool overwritePreviousScissor);
    void setDepthInfo(const DepthInfo& depthInfo);
    void setStencilInfo(const StencilInfo& stencilInfo);
    void setWrapMode(textures::WrapMode wrapModeS, textures::WrapMode wrapModeT);
    void setFrameBuffer(IFrameBuffer* frameBuffer, bool force = false);
    void setShader(shaders::IShader* shader);
};

} // namespace panlid::graphics::rendering
