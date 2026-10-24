#pragma once

#include "panlid/graphics/BlendingMask.hh"
#include "panlid/graphics/BlendingParameters.hh"
#include "panlid/graphics/Colour4.hh"
#include "panlid/graphics/primitives/RectangleI.hh"
#include "panlid/graphics/rendering/ClearInfo.hh"
#include "panlid/graphics/rendering/DepthInfo.hh"
#include "panlid/graphics/rendering/IFrameBuffer.hh"
#include "panlid/graphics/rendering/INativeTexture.hh"
#include "panlid/graphics/rendering/IUniformBuffer.hh"
#include "panlid/graphics/rendering/IVertexBatch.hh"
#include "panlid/graphics/rendering/PrimitiveTopology.hh"
#include "panlid/graphics/rendering/Renderer.hh"
#include "panlid/graphics/rendering/StencilInfo.hh"
#include "panlid/graphics/rendering/dummy/DummyUniformBuffer.hh"
#include "panlid/graphics/rendering/dummy/DummyVertexBatch.hh"
#include "panlid/graphics/rendering/vertices/VertexMember.hh"
#include "panlid/graphics/shaders/IShader.hh"
#include "panlid/graphics/textures/TextureFilteringMode.hh"
#include "panlid/platform/IGraphicsSurface.hh"
#include "panlid/platform/sdl3/Sdl3Exception.hh"
#include "panlid/platform/sdl3/Sdl3Extensions.hh"
#include "panlid/platform/sdl3/Sdl3GraphicsSurface.hh"
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_video.h>
#include <cstddef>
#include <glm/vec2.hpp>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>

namespace panlid::graphics::rendering::sdl3 {

// A renderer that draws with SDL's GPU API. Only clearing the backbuffer works so far. Drawing,
// shaders, textures and framebuffers are rejected, and the state setters do nothing.
//
// Destroy it before the window it was initialised with.
//
// todo: blending, viewport, scissor, depth and stencil state, uniform buffers, vertex batches,
// shaders, textures, framebuffers and drawing.
class Sdl3GpuRenderer final : public Renderer {
  public:
    ~Sdl3GpuRenderer() override {
        if (device_ == nullptr) {
            return;
        }

        platform::sdl3::LogErrorIfFailed(SDL_WaitForGPUIdle(device_));

        if (window_ != nullptr) {
            SDL_ReleaseWindowFromGPUDevice(device_, window_);
        }

        SDL_DestroyGPUDevice(device_);
    }

    // SDL's GPU API uses the same conventions on every backend.
    [[nodiscard]] bool IsDepthRangeZeroToOne() const override { return true; }
    [[nodiscard]] bool IsUvOriginTopLeft() const override { return true; }
    [[nodiscard]] bool IsClipSpaceYInverted() const override { return false; }

    // todo: GameHost drives the frame lifecycle. Make these protected again once it exists.
    using Renderer::FinishFrame;
    using Renderer::Initialise;

    // Acquires the frame's command buffer and swapchain texture before the base class resets its
    // state, because that clears the backbuffer.
    void BeginFrame(const glm::vec2& windowSize) override {
        commandBuffer_ = SDL_AcquireGPUCommandBuffer(device_);

        if (commandBuffer_ == nullptr) {
            throw platform::sdl3::Sdl3Exception{};
        }

        // The texture stays null while the window is hidden or minimised. Clears are skipped then.
        platform::sdl3::ThrowIfFailed(SDL_WaitAndAcquireGPUSwapchainTexture(
            commandBuffer_, window_, &swapchainTexture_, nullptr, nullptr));

        Renderer::BeginFrame(windowSize);
    }

    // Submitting a command buffer that acquired the swapchain texture also presents it.
    void SwapBuffers() override {
        platform::sdl3::LogErrorIfFailed(SDL_SubmitGPUCommandBuffer(commandBuffer_));

        commandBuffer_ = nullptr;
        swapchainTexture_ = nullptr;
    }

  protected:
    [[nodiscard]] bool GetVerticalSync() const override { return verticalSync_; }

    void SetVerticalSync(bool verticalSync) override {
        verticalSync_ = verticalSync;
        applyPresentMode();
    }

    [[nodiscard]] bool GetAllowTearing() const override { return allowTearing_; }

    void SetAllowTearing(bool allowTearing) override {
        allowTearing_ = allowTearing;
        applyPresentMode();
    }

    void WaitUntilIdle() override { platform::sdl3::LogErrorIfFailed(SDL_WaitForGPUIdle(device_)); }

    // Acquiring the swapchain texture in BeginFrame already waits for the next frame.
    void WaitUntilNextFrameReady() override {}

    // SDL's GPU API has no current context.
    void MakeCurrent() override {}
    void ClearCurrent() override {}

    void InitialiseImplementation(platform::IGraphicsSurface& graphicsSurface) override {
        // todo: remove this cast once surface types exist.
        auto* surface = dynamic_cast<platform::sdl3::Sdl3GraphicsSurface*>(&graphicsSurface);

        if (surface == nullptr) {
            throw std::invalid_argument("The SDL3 GPU renderer needs an SDL3 graphics surface.");
        }

        device_ = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, kDebugMode, "vulkan");

        if (device_ == nullptr) {
            throw platform::sdl3::Sdl3Exception{};
        }

        SDL_Window* window = surface->GetSdlWindowHandle();
        platform::sdl3::ThrowIfFailed(SDL_ClaimWindowForGPUDevice(device_, window));
        window_ = window;

        applyPresentMode();
    }

    void ClearImplementation(const ClearInfo& clearInfo) override {
        if (commandBuffer_ == nullptr) {
            throw std::runtime_error("Cannot clear outside of a frame.");
        }

        if (swapchainTexture_ == nullptr) {
            return;
        }

        // todo: clear the depth and stencil values, once there is a depth target.
        SDL_GPUColorTargetInfo target{};
        target.texture = swapchainTexture_;
        target.clear_color = SDL_FColor{.r = clearInfo.Colour.R,
                                        .g = clearInfo.Colour.G,
                                        .b = clearInfo.Colour.B,
                                        .a = clearInfo.Colour.A};
        target.load_op = SDL_GPU_LOADOP_CLEAR;
        target.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(commandBuffer_, &target, 1, nullptr);

        if (pass == nullptr) {
            throw platform::sdl3::Sdl3Exception{};
        }

        SDL_EndGPURenderPass(pass);
    }

    void SetBlendImplementation(const BlendingParameters& /*blendingParameters*/) override {}
    void SetBlendMaskImplementation(BlendingMask /*blendingMask*/) override {}
    void SetViewportImplementation(const primitives::RectangleI& /*viewport*/) override {}
    void SetScissorImplementation(const primitives::RectangleI& /*scissor*/) override {}
    void SetScissorStateImplementation(bool /*enabled*/) override {}
    void SetDepthInfoImplementation(const DepthInfo& /*depthInfo*/) override {}
    void SetStencilInfoImplementation(const StencilInfo& /*stencilInfo*/) override {}

    bool SetTextureImplementation(INativeTexture* texture, int /*unit*/) override {
        if (texture != nullptr) {
            throw std::runtime_error("Textures are not supported yet.");
        }

        return true;
    }

    void SetFrameBufferImplementation(IFrameBuffer* frameBuffer) override {
        if (frameBuffer != nullptr) {
            throw std::runtime_error("Framebuffers are not supported yet.");
        }
    }

    // No framebuffer can exist yet, so there is nothing to delete.
    void DeleteFrameBufferImplementation(IFrameBuffer& /*frameBuffer*/) override {}

    void DrawVerticesImplementation(PrimitiveTopology /*topology*/, int /*vertexStart*/,
                                    int /*verticesCount*/) override {
        throw std::runtime_error("Drawing is not supported yet.");
    }

    void SetShaderImplementation(shaders::IShader& /*shader*/) override {
        throw std::runtime_error("Shaders are not supported yet.");
    }

    void SetUniformBufferImplementation(std::string_view /*blockName*/,
                                        IUniformBuffer& /*buffer*/) override {}

    std::unique_ptr<IVertexBatch>
    CreateLinearBatchImplementation(std::span<const vertices::VertexMember> /*layout*/,
                                    std::size_t /*vertexSize*/, int /*size*/, int /*maxBuffers*/,
                                    PrimitiveTopology /*topology*/) override {
        return std::make_unique<dummy::DummyVertexBatch>();
    }

    std::unique_ptr<IVertexBatch>
    CreateQuadBatchImplementation(std::span<const vertices::VertexMember> /*layout*/,
                                  std::size_t /*vertexSize*/, int /*size*/,
                                  int /*maxBuffers*/) override {
        return std::make_unique<dummy::DummyVertexBatch>();
    }

    std::unique_ptr<IUniformBuffer>
    CreateUniformBufferImplementation(std::size_t /*size*/) override {
        return std::make_unique<dummy::DummyUniformBuffer>();
    }

    std::unique_ptr<INativeTexture>
    CreateNativeTexture(int /*width*/, int /*height*/, bool /*manualMipmaps*/,
                        textures::TextureFilteringMode /*filteringMode*/,
                        const std::optional<Colour4>& /*initialisationColour*/) override {
        throw std::runtime_error("Textures are not supported yet.");
    }

    std::unique_ptr<INativeTexture> CreateNativeVideoTexture(int /*width*/,
                                                             int /*height*/) override {
        throw std::runtime_error("Video textures are not supported yet.");
    }

  private:
    // todo: enable the validation layers in debug builds.
    static constexpr bool kDebugMode = false;

    SDL_GPUDevice* device_ = nullptr;
    SDL_Window* window_ = nullptr;
    SDL_GPUCommandBuffer* commandBuffer_ = nullptr;
    SDL_GPUTexture* swapchainTexture_ = nullptr;
    bool verticalSync_ = true;
    bool allowTearing_ = false;

    void applyPresentMode() {
        if (device_ == nullptr || window_ == nullptr) {
            return;
        }

        SDL_GPUPresentMode mode = SDL_GPU_PRESENTMODE_VSYNC;

        if (!verticalSync_) {
            mode = allowTearing_ ? SDL_GPU_PRESENTMODE_IMMEDIATE : SDL_GPU_PRESENTMODE_MAILBOX;
        }

        // VSYNC is the one mode every window supports.
        if (!SDL_WindowSupportsGPUPresentMode(device_, window_, mode)) {
            mode = SDL_GPU_PRESENTMODE_VSYNC;
        }

        platform::sdl3::LogErrorIfFailed(SDL_SetGPUSwapchainParameters(
            device_, window_, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, mode));
    }
};

} // namespace panlid::graphics::rendering::sdl3
