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

class IGraphicsSurface;

} // namespace panlid::platform

namespace panlid::graphics::rendering {

class IFrameBuffer;

// Draws to the screen. Backends derive from this class and implement the *Implementation hooks.
//
// This merges the C# IRenderer interface and Renderer base class, since Renderer is the only
// implementer. The renderer keeps non-owning pointers to batches, shaders, framebuffers, textures
// and uniform buffers: whoever created one keeps it alive for as long as the renderer can still
// reach it (until it is popped or unbound, and for a batch until the end of the frame it was last
// active in).
class Renderer {
  public:
    // Maximum number of DrawNodes a Drawable can draw with. This is a carefully-chosen number to
    // let the update and draw threads work concurrently without causing unnecessary load.
    static constexpr int kMaxDrawNodes = 3;
    static constexpr int kMaxMipmapLevels = 3;
    // Not ported: C#'s VERTICES_PER_TRIANGLE (= 4, which is wrong and unused).
    static constexpr int kVerticesPerQuad = 4;
    static constexpr int kIndicesPerQuad = kVerticesPerQuad + 2;
    // Maximum number of vertices in a linear vertex buffer (ushort.MaxValue in C#).
    static constexpr int kMaxVertices = 65535;
    static constexpr int kMaxQuads = kMaxVertices / kIndicesPerQuad;
    // The length of no usage (in frames) before unused resources are freed.
    static constexpr uint64_t kResourceFreeNoUsageLength = 300;

    Renderer();
    Renderer(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer& operator=(Renderer&&) = delete;
    virtual ~Renderer() = default;

    // Enables or disables vertical sync.
    [[nodiscard]] virtual bool GetVerticalSync() const = 0;
    virtual void SetVerticalSync(bool verticalSync) = 0;

    [[nodiscard]] virtual bool GetAllowTearing() const = 0;
    virtual void SetAllowTearing(bool allowTearing) = 0;

    // todo: CacheStorage (shader compilation cache) once Storage and ShaderCompilationStore are
    // ported.

    [[nodiscard]] uint64_t GetFrameIndex() const { return frameIndex_; }

    // The maximum allowed texture size.
    [[nodiscard]] int GetMaxTextureSize() const { return maxTextureSize_; }

    // The maximum number of texture uploads to dequeue and upload per frame.
    [[nodiscard]] int GetMaxTexturesUploadedPerFrame() const {
        return maxTexturesUploadedPerFrame_;
    }
    void SetMaxTexturesUploadedPerFrame(int count) { maxTexturesUploadedPerFrame_ = count; }

    // The maximum number of pixels to upload per frame. Defaults to 2 megapixels (8mb alloc).
    [[nodiscard]] int GetMaxPixelsUploadedPerFrame() const { return maxPixelsUploadedPerFrame_; }
    void SetMaxPixelsUploadedPerFrame(int count) { maxPixelsUploadedPerFrame_ = count; }

    // Whether the depth is in the range [0, 1] (i.e. Reversed-Z). If false, depth is in the range
    // [-1, 1].
    [[nodiscard]] virtual bool IsDepthRangeZeroToOne() const = 0;

    // Whether the texture coordinates begin in the top-left of the texture. If false, (0, 0) is the
    // bottom-left texel.
    [[nodiscard]] virtual bool IsUvOriginTopLeft() const = 0;

    // Whether the y-coordinate ranges from -1 (top) to 1 (bottom). If false, it ranges from -1
    // (bottom) to 1 (top).
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
    // Whether the currently bound framebuffer is the backbuffer.
    [[nodiscard]] bool IsUsingBackbuffer() const { return usingBackbuffer_; }
    // todo: WhitePixel texture (needs Texture and TextureAtlas).

    // The current depth of TexturedVertex2D vertices when drawn to the backbuffer.
    // todo: DrawNodes also mutate this (Increment/Set); decide that API when DrawNode is ported.
    [[nodiscard]] const DepthValue& GetBackbufferDepth() const { return backbufferDepth_; }

    [[nodiscard]] bool IsInitialised() const { return isInitialised_; }
    [[nodiscard]] BlendingParameters GetCurrentBlendingParameters() const {
        return currentBlendingParameters_;
    }

    // Performs a once-off initialisation of this renderer.
    void Initialise(platform::IGraphicsSurface& graphicsSurface);

    // Resets any states to prepare for drawing a new frame. windowSize is the full window size.
    virtual void BeginFrame(const glm::vec2& windowSize);

    // Performs any last actions before a frame ends.
    virtual void FinishFrame();

    // Swaps the back buffer with the front buffer to display the new frame.
    virtual void SwapBuffers() = 0;

    // Waits until all renderer commands have been fully executed GPU-side (equivalent to glFinish).
    virtual void WaitUntilIdle() = 0;

    // Waits until the GPU signals that the next frame is ready to be rendered.
    virtual void WaitUntilNextFrameReady() = 0;

    // Invoked when the rendering thread is active and commands will be enqueued.
    virtual void MakeCurrent() = 0;

    // Invoked when the rendering thread is suspended and no more commands will be enqueued.
    virtual void ClearCurrent() = 0;

    // Schedules an expensive operation to a queue from which a maximum of one operation is
    // performed per frame.
    void ScheduleExpensiveOperation(std::shared_ptr<threading::ScheduledDelegate> operation);

    // Schedules a disposal action to be run a few frames from now (immediately if not yet
    // initialised). The action must own everything it touches: capture shared_ptrs or moved-in
    // objects, never raw pointers or references to objects that may be destroyed before it runs.
    void ScheduleDisposal(std::function<void()> disposalAction);

    // todo: TakeScreenshot and ExtractFrameBufferData (need an image library).

    // Clears the currently bound frame buffer.
    void Clear(const ClearInfo& clearInfo);

    void SetBlend(const BlendingParameters& blendingParameters);

    // Sets a mask deciding which colour components are affected during blending.
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

    // Notifies that a vertex buffer has begun being used, so it can be freed after going unused for
    // a while.
    void RegisterVertexBufferUse(const std::shared_ptr<IVertexBuffer>& buffer);

    // Sets the last vertex batch used for drawing, so renderer state changes can force-draw it
    // first.
    void SetActiveBatch(IVertexBatch& batch);

    // Flushes the currently active vertex batch. source is for profiling purposes.
    void FlushCurrentBatch(std::optional<FlushBatchSource> source);

    // Binds a native texture. Returns whether the texture was bound.
    // todo: BindTexture(Texture, unit, optional wrap modes) once Texture is ported.
    bool BindTexture(INativeTexture& texture, int unit = 0,
                     textures::WrapMode wrapModeS = textures::WrapMode::None,
                     textures::WrapMode wrapModeT = textures::WrapMode::None);

    void UnbindTexture(int unit = 0);

    // Enqueues a texture to be uploaded in the next frame. The texture must stay alive while
    // queued. todo: a texture should remove itself from the queue when destroyed.
    void EnqueueTextureUpload(INativeTexture& texture);

    void BindFrameBuffer(IFrameBuffer& frameBuffer);
    void UnbindFrameBuffer(IFrameBuffer* frameBuffer);

    // Deletes a frame buffer. The renderer keeps it alive until the delayed deletion has run.
    void DeleteFrameBuffer(std::shared_ptr<IFrameBuffer> frameBuffer);

    void DrawVertices(PrimitiveTopology topology, int vertexStart, int verticesCount);

    void BindShader(shaders::IShader& shader);
    void UnbindShader(shaders::IShader& shader);
    // todo: SetUniform (needs IUniform/IUniformWithValue).

    void BindUniformBuffer(std::string_view blockName, IUniformBuffer& buffer);

    // todo: CreateShaderPart, CreateShader, GetMipmapShader (need IShaderStore,
    // ShaderCompilationStore). todo: CreateFrameBuffer, CreateTexture, CreateVideoTexture,
    // texture-visualiser support (need IFrameBuffer, Texture). todo:
    // CreateShaderStorageBufferObject.

    // Creates a new linear vertex batch, accepting vertices and drawing as a given primitive type.
    // size is the number of vertices per buffer, maxBuffers the maximum number of vertex buffers.
    template <vertices::Vertex T>
    [[nodiscard]] std::unique_ptr<IVertexBatch> CreateLinearBatch(int size, int maxBuffers,
                                                                  PrimitiveTopology topology) {
        validateLinearBatch(size, maxBuffers);
        const auto layout = T::GetLayout();
        return CreateLinearBatchImplementation(layout, sizeof(T), size, maxBuffers, topology);
    }

    // Creates a new quad vertex batch, accepting vertices and drawing as quads. size is the number
    // of quads.
    template <vertices::Vertex T>
    [[nodiscard]] std::unique_ptr<IVertexBatch> CreateQuadBatch(int size, int maxBuffers) {
        validateQuadBatch(size, maxBuffers);
        const auto layout = T::GetLayout();
        return CreateQuadBatchImplementation(layout, sizeof(T), size, maxBuffers);
    }

    // Creates a buffer that stores data for a uniform block of a shader.
    template <typename TData>
        requires std::is_trivially_copyable_v<TData>
    [[nodiscard]] std::unique_ptr<IUniformBuffer> CreateUniformBuffer() {
        // std140 requires the block size to be a multiple of 16 (C# checked this by reflection at
        // runtime).
        static_assert(sizeof(TData) % kUniformBlockAlignment == 0,
                      "uniform block data must be padded to a multiple of 16 bytes");
        return CreateUniformBufferImplementation(sizeof(TData));
    }

  protected:
    void SetMaxTextureSize(int size) { maxTextureSize_ = size; }

    // Whether scissor is currently enabled.
    [[nodiscard]] bool IsScissorEnabled() const { return scissorEnabled_; }
    [[nodiscard]] ClearInfo GetCurrentClearInfo() const { return currentClearInfo_; }
    [[nodiscard]] BlendingMask GetCurrentBlendingMask() const { return currentBlendingMask_; }

    // The current framebuffer, or null if the backbuffer is used.
    [[nodiscard]] IFrameBuffer* GetFrameBuffer() const { return frameBuffer_; }

    // The current shader, or null if no shader is currently bound.
    [[nodiscard]] shaders::IShader* GetShader() const { return shader_; }

    virtual void InitialiseImplementation(platform::IGraphicsSurface& graphicsSurface) = 0;

    // Clears the color and depth targets of the currently bound framebuffer.
    virtual void ClearImplementation(const ClearInfo& clearInfo) = 0;

    virtual void SetBlendImplementation(const BlendingParameters& blendingParameters) = 0;
    virtual void SetBlendMaskImplementation(BlendingMask blendingMask) = 0;
    virtual void SetViewportImplementation(const primitives::RectangleI& viewport) = 0;
    virtual void SetScissorImplementation(const primitives::RectangleI& scissor) = 0;
    virtual void SetScissorStateImplementation(bool enabled) = 0;
    virtual void SetDepthInfoImplementation(const DepthInfo& depthInfo) = 0;
    virtual void SetStencilInfoImplementation(const StencilInfo& stencilInfo) = 0;

    // Uses the given texture for drawing (null means the default texture). Returns whether it was
    // set successfully.
    virtual bool SetTextureImplementation(INativeTexture* texture, int unit) = 0;

    // Uses the given framebuffer for drawing (null means the backbuffer).
    virtual void SetFrameBufferImplementation(IFrameBuffer* frameBuffer) = 0;

    // todo: the deletion runs frames later, so a backend frame buffer should keep its GPU handles
    // in a separately owned object. Decide that with the first real backend.
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

    // C# gave the optional arguments defaults here; callers now pass them explicitly.
    virtual std::unique_ptr<INativeTexture>
    CreateNativeTexture(int width, int height, bool manualMipmaps,
                        textures::TextureFilteringMode filteringMode,
                        const std::optional<Colour4>& initialisationColour) = 0;
    virtual std::unique_ptr<INativeTexture> CreateNativeVideoTexture(int width, int height) = 0;

  private:
    static constexpr int kMaxTextureUnits = 16;
    static constexpr std::size_t kUniformBlockAlignment = 16;

    // State read from other threads.
    std::atomic<bool> isInitialised_ = false;
    std::atomic<uint64_t> frameIndex_ = 0;
    // The default allows roughly normal flow when there is no graphics context, like headless CI.
    std::atomic<int> maxTextureSize_ = 4096;
    std::atomic<int> maxTexturesUploadedPerFrame_ = 32;
    std::atomic<int> maxPixelsUploadedPerFrame_ = 1024 * 1024 * 2;

    // Renderer state. It starts all-zero like C#'s default(T), not at the C++ Default() values: the
    // first frame's pushes must differ from it, so the backend is told the initial state.
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

    // Declared in this order because the constructor initialises them in this order.
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

    // todo: replace with PushOrtho from RendererExtensions once that is ported.
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
