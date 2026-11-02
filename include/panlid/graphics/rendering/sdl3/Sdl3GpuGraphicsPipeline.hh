#pragma once

#include "panlid/graphics/rendering/PrimitiveTopology.hh"
#include "panlid/graphics/rendering/sdl3/Sdl3GpuShader.hh"
#include "panlid/graphics/rendering/vertices/VertexMember.hh"
#include "panlid/platform/sdl3/Sdl3Exception.hh"
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace panlid::graphics::rendering::sdl3 {

// Holds the state needed to draw and turns it into SDL render passes. SDL pipelines are immutable,
// so one is created for each combination of state that is drawn with, and cached.
//
// Destroy it before the device it was created with.
//
// todo: blending, depth and stencil state, viewport and scissor, textures, uniform buffers, index
// buffers and framebuffers.
// todo: draw a frame's vertices in one render pass instead of one pass per draw.
// todo: free the pipelines of destroyed shaders.
class Sdl3GpuGraphicsPipeline {
  public:
    Sdl3GpuGraphicsPipeline(SDL_GPUDevice* device, SDL_Window* window)
        : device_{device}, window_{window} {}

    Sdl3GpuGraphicsPipeline(const Sdl3GpuGraphicsPipeline&) = delete;
    Sdl3GpuGraphicsPipeline& operator=(const Sdl3GpuGraphicsPipeline&) = delete;
    Sdl3GpuGraphicsPipeline(Sdl3GpuGraphicsPipeline&&) = delete;
    Sdl3GpuGraphicsPipeline& operator=(Sdl3GpuGraphicsPipeline&&) = delete;

    ~Sdl3GpuGraphicsPipeline() {
        for (const auto& entry : pipelineCache_) {
            SDL_ReleaseGPUGraphicsPipeline(device_, entry.second);
        }
    }

    // Starts a frame that draws to the swapchain texture, which is null while the window is hidden
    // or minimised. Draws are skipped then.
    void Begin(SDL_GPUCommandBuffer* commandBuffer, SDL_GPUTexture* swapchainTexture) {
        commandBuffer_ = commandBuffer;
        swapchainTexture_ = swapchainTexture;
        targetFormat_ = SDL_GetGPUSwapchainTextureFormat(device_, window_);

        shaderId_ = 0;
        vertexShader_ = nullptr;
        fragmentShader_ = nullptr;
        vertexBuffer_ = nullptr;
    }

    // Drops the frame's command buffer and texture. The caller submits the command buffer.
    void End() {
        commandBuffer_ = nullptr;
        swapchainTexture_ = nullptr;
    }

    // The shader must stay alive until the draws that use it have been issued.
    void SetShader(const Sdl3GpuShader& shader) {
        shaderId_ = shader.GetId();
        vertexShader_ = shader.GetVertexShader();
        fragmentShader_ = shader.GetFragmentShader();
    }

    // The layout is copied.
    void SetVertexBuffer(SDL_GPUBuffer* buffer, std::span<const vertices::VertexMember> layout,
                         std::size_t vertexSize) {
        vertexBuffer_ = buffer;

        if (vertexSize_ != vertexSize || !std::ranges::equal(layout, vertexLayout_)) {
            vertexLayout_.assign(layout.begin(), layout.end());
            vertexSize_ = vertexSize;
        }

        // todo: count the buffer binds in the frame statistics.
    }

    // Draws vertices from the active vertex buffer.
    void DrawVertices(PrimitiveTopology topology, int vertexStart, int verticesCount) {
        if (vertexShader_ == nullptr) {
            throw std::runtime_error("No shader bound.");
        }

        if (vertexBuffer_ == nullptr) {
            throw std::runtime_error("No vertex buffer bound.");
        }

        if (commandBuffer_ == nullptr) {
            throw std::runtime_error("Cannot draw outside of a frame.");
        }

        if (swapchainTexture_ == nullptr) {
            return;
        }

        SDL_GPUGraphicsPipeline* pipeline = pipelineFor(topology);

        // Keep what the frame has drawn so far, including the clear.
        SDL_GPUColorTargetInfo target{};
        target.texture = swapchainTexture_;
        target.load_op = SDL_GPU_LOADOP_LOAD;
        target.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(commandBuffer_, &target, 1, nullptr);

        if (pass == nullptr) {
            throw platform::sdl3::Sdl3Exception{};
        }

        SDL_BindGPUGraphicsPipeline(pass, pipeline);

        const SDL_GPUBufferBinding binding{.buffer = vertexBuffer_, .offset = 0};
        SDL_BindGPUVertexBuffers(pass, 0, &binding, 1);

        // todo: draw indexed once the index buffer is ported.
        SDL_DrawGPUPrimitives(pass, static_cast<std::uint32_t>(verticesCount), 1,
                              static_cast<std::uint32_t>(vertexStart), 0);

        SDL_EndGPURenderPass(pass);
    }

  private:
    // Everything a pipeline depends on. The shader is identified by an id, because a destroyed
    // shader's address can be reused by a different one.
    struct PipelineKey {
        std::uint64_t ShaderId{};
        PrimitiveTopology Topology{};
        std::vector<vertices::VertexMember> Layout{};
        std::size_t VertexSize{};
        SDL_GPUTextureFormat TargetFormat{};

        auto operator<=>(const PipelineKey&) const = default;
    };

    SDL_GPUDevice* device_;
    SDL_Window* window_;
    SDL_GPUCommandBuffer* commandBuffer_ = nullptr;
    SDL_GPUTexture* swapchainTexture_ = nullptr;
    SDL_GPUTextureFormat targetFormat_ = SDL_GPU_TEXTUREFORMAT_INVALID;

    std::uint64_t shaderId_ = 0;
    SDL_GPUShader* vertexShader_ = nullptr;
    SDL_GPUShader* fragmentShader_ = nullptr;

    SDL_GPUBuffer* vertexBuffer_ = nullptr;
    std::vector<vertices::VertexMember> vertexLayout_;
    std::size_t vertexSize_ = 0;

    std::map<PipelineKey, SDL_GPUGraphicsPipeline*> pipelineCache_;

    // todo: the key copies the layout on every draw. Identify layouts by an id instead.
    SDL_GPUGraphicsPipeline* pipelineFor(PrimitiveTopology topology) {
        PipelineKey key{.ShaderId = shaderId_,
                        .Topology = topology,
                        .Layout = vertexLayout_,
                        .VertexSize = vertexSize_,
                        .TargetFormat = targetFormat_};

        if (const auto found = pipelineCache_.find(key); found != pipelineCache_.end()) {
            return found->second;
        }

        std::vector<SDL_GPUVertexAttribute> attributes;
        attributes.reserve(vertexLayout_.size());

        // Member i feeds the shader input at location i.
        for (std::size_t i = 0; i < vertexLayout_.size(); ++i) {
            attributes.push_back(SDL_GPUVertexAttribute{
                .location = static_cast<std::uint32_t>(i),
                .buffer_slot = 0,
                .format = toVertexElementFormat(vertexLayout_[i]),
                .offset = static_cast<std::uint32_t>(vertexLayout_[i].Offset)});
        }

        const SDL_GPUVertexBufferDescription bufferDescription{
            .slot = 0,
            .pitch = static_cast<std::uint32_t>(vertexSize_),
            .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
            .instance_step_rate = 0};

        // Blending is off and every colour channel is written.
        // todo: blending, once BlendingParameters can be mapped.
        SDL_GPUColorTargetDescription colorTarget{};
        colorTarget.format = targetFormat_;

        // No culling, no depth or stencil test, no multisampling.
        SDL_GPUGraphicsPipelineCreateInfo info{};
        info.vertex_shader = vertexShader_;
        info.fragment_shader = fragmentShader_;
        info.vertex_input_state.vertex_buffer_descriptions = &bufferDescription;
        info.vertex_input_state.num_vertex_buffers = 1;
        info.vertex_input_state.vertex_attributes = attributes.data();
        info.vertex_input_state.num_vertex_attributes =
            static_cast<std::uint32_t>(attributes.size());
        info.primitive_type = toPrimitiveType(topology);
        info.target_info.color_target_descriptions = &colorTarget;
        info.target_info.num_color_targets = 1;

        SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device_, &info);

        if (pipeline == nullptr) {
            throw platform::sdl3::Sdl3Exception{};
        }

        pipelineCache_.emplace(std::move(key), pipeline);
        // todo: count the created pipelines in the statistics.

        return pipeline;
    }

    static SDL_GPUVertexElementFormat toVertexElementFormat(const vertices::VertexMember& member) {
        // todo: the other component types, once a vertex needs them.
        if (member.Type != vertices::VertexComponentType::Float) {
            throw std::invalid_argument("Only float vertex members are supported.");
        }

        switch (member.Count) {
        case 1:
            return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
        case 2:
            return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        case 3:
            return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        case 4:
            return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
        default:
            break;
        }

        throw std::invalid_argument("A vertex member needs between 1 and 4 components.");
    }

    static SDL_GPUPrimitiveType toPrimitiveType(PrimitiveTopology topology) {
        switch (topology) {
        case PrimitiveTopology::Points:
            return SDL_GPU_PRIMITIVETYPE_POINTLIST;
        case PrimitiveTopology::Lines:
            return SDL_GPU_PRIMITIVETYPE_LINELIST;
        case PrimitiveTopology::LineStrip:
            return SDL_GPU_PRIMITIVETYPE_LINESTRIP;
        case PrimitiveTopology::Triangles:
            return SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        case PrimitiveTopology::TriangleStrip:
            return SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP;
        }

        throw std::invalid_argument("Unknown primitive topology.");
    }
};

} // namespace panlid::graphics::rendering::sdl3
