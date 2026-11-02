#pragma once

#include "panlid/graphics/rendering/IUniformBuffer.hh"
#include "panlid/graphics/rendering/Renderer.hh"
#include "panlid/graphics/shaders/IShader.hh"
#include "panlid/platform/sdl3/Sdl3Exception.hh"
#include <SDL3/SDL_gpu.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace panlid::graphics::rendering::sdl3 {

// A vertex and fragment shader pair on an SDL GPU device. Both stages are created in the
// constructor.
//
// Destroy it before the renderer that owns the device it was created on.
//
// todo: build the stages from shader parts and a compilation store, and create them on the draw
// thread through the renderer's expensive-operation queue.
class Sdl3GpuShader final : public shaders::IShader {
  public:
    // One compiled stage. The counts must match the resources the SPIR-V declares.
    // todo: read the counts from the SPIR-V instead of passing them.
    struct StageSource {
        std::vector<std::uint32_t> Spirv{};
        std::uint32_t NumSamplers{};
        std::uint32_t NumUniformBuffers{};
    };

    Sdl3GpuShader(Renderer& renderer, SDL_GPUDevice* device, std::string name,
                  const StageSource& vertex, const StageSource& fragment)
        : renderer_{renderer}, name_{std::move(name)},
          vertexShader_{createShader(device, SDL_GPU_SHADERSTAGE_VERTEX, vertex),
                        ShaderDeleter{device}},
          fragmentShader_{createShader(device, SDL_GPU_SHADERSTAGE_FRAGMENT, fragment),
                          ShaderDeleter{device}} {}

    [[nodiscard]] const std::string& GetName() const { return name_; }

    // Unique among all shaders, even after one is destroyed.
    [[nodiscard]] std::uint64_t GetId() const { return id_; }

    void Bind() override {
        if (isBound_) {
            return;
        }

        renderer_.BindShader(*this);
        isBound_ = true;
    }

    void Unbind() override {
        if (!isBound_) {
            return;
        }

        renderer_.UnbindShader(*this);
        isBound_ = false;
    }

    // Both stages exist once the constructor returns.
    [[nodiscard]] bool IsLoaded() const override { return true; }
    [[nodiscard]] bool IsBound() const override { return isBound_; }

    void BindUniformBlock(std::string_view blockName, IUniformBuffer& buffer) override {
        renderer_.BindUniformBuffer(blockName, buffer);
    }

    [[nodiscard]] SDL_GPUShader* GetVertexShader() const { return vertexShader_.get(); }
    [[nodiscard]] SDL_GPUShader* GetFragmentShader() const { return fragmentShader_.get(); }

  private:
    struct ShaderDeleter {
        SDL_GPUDevice* Device{};

        void operator()(SDL_GPUShader* shader) const noexcept {
            SDL_ReleaseGPUShader(Device, shader);
        }
    };

    Renderer& renderer_;
    std::string name_;
    std::unique_ptr<SDL_GPUShader, ShaderDeleter> vertexShader_;
    std::unique_ptr<SDL_GPUShader, ShaderDeleter> fragmentShader_;
    bool isBound_ = false;
    const std::uint64_t id_ = nextId();

    static SDL_GPUShader* createShader(SDL_GPUDevice* device, SDL_GPUShaderStage stage,
                                       const StageSource& source) {
        if (source.Spirv.empty()) {
            throw std::invalid_argument("A shader stage needs SPIR-V code.");
        }

        SDL_GPUShaderCreateInfo info{};
        info.code_size = source.Spirv.size() * sizeof(std::uint32_t);
        info.code = reinterpret_cast<const std::uint8_t*>(source.Spirv.data());
        // todo: the entry point is "main0" for MSL.
        info.entrypoint = "main";
        info.format = SDL_GPU_SHADERFORMAT_SPIRV;
        info.stage = stage;
        info.num_samplers = source.NumSamplers;
        info.num_uniform_buffers = source.NumUniformBuffers;

        SDL_GPUShader* shader = SDL_CreateGPUShader(device, &info);

        if (shader == nullptr) {
            throw platform::sdl3::Sdl3Exception{};
        }

        return shader;
    }

    static std::uint64_t nextId() {
        static std::atomic<std::uint64_t> next{1};
        return next++;
    }
};

} // namespace panlid::graphics::rendering::sdl3
