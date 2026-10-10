// Spike: fake update thread -> TripleBuffer -> window/draw thread (SDL_GPU).
#include "panlid/allocation/TripleBuffer.hh"

#include <SDL3/SDL.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <stop_token>
#include <string>
#include <thread>

using namespace std::chrono_literals;
using panlid::allocation::TripleBuffer;

namespace {

struct SceneState {
    float Angle = 0.0F;
    std::uint64_t Tick = 0;
};

// Must match the uniform block in anim.vert (std140 -> 16 bytes).
struct VertexParams {
    float angle;
    float aspect;
    float padding[2];
};

SDL_GPUShader* loadShader(SDL_GPUDevice* device, const char* name, SDL_GPUShaderStage stage,
                          Uint32 numUniformBuffers) {
    const std::string path = std::string(SPIKE_SHADER_DIR) + "/" + name;

    std::size_t size = 0;
    void* code = SDL_LoadFile(path.c_str(), &size);
    if (code == nullptr) {
        std::fprintf(stderr, "could not read %s: %s\n", path.c_str(), SDL_GetError());
        return nullptr;
    }

    SDL_GPUShaderCreateInfo info{};
    info.code = static_cast<const Uint8*>(code);
    info.code_size = size;
    info.entrypoint = "main";
    info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    info.stage = stage;
    info.num_uniform_buffers = numUniformBuffers;

    SDL_GPUShader* shader = SDL_CreateGPUShader(device, &info);
    SDL_free(code);
    return shader;
}

// The fake "update thread": produces a new state far more often than we draw.
void updateLoop(std::stop_token stop, TripleBuffer<SceneState>& buffer,
                std::atomic<std::uint64_t>& ticksProduced) {
    using clock = std::chrono::steady_clock;
    const auto start = clock::now();
    std::uint64_t tick = 0;

    while (!stop.stop_requested()) {
        const float seconds = std::chrono::duration<float>(clock::now() - start).count();
        ++tick;
        {
            auto write = buffer.GetForWrite();
            write.SetObject(SceneState{.Angle = seconds * 1.5F, .Tick = tick});
        }
        ticksProduced.store(tick, std::memory_order_relaxed);
        std::this_thread::sleep_for(1ms);
    }
}

} // namespace

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    // This thread creates the window, so it is the only thread allowed to
    // acquire the swapchain. That makes it the "draw thread" in this design.
    SDL_Window* window = SDL_CreateWindow("panlid thread spike", 960, 540, SDL_WINDOW_RESIZABLE);
    if (window == nullptr) {
        std::fprintf(stderr, "window creation failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GPUDevice* device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, nullptr);
    if (device == nullptr || !SDL_ClaimWindowForGPUDevice(device, window)) {
        std::fprintf(stderr, "GPU setup failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GPUShader* vertexShader = loadShader(device, "anim.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 1);
    SDL_GPUShader* fragmentShader =
        loadShader(device, "anim.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 0);
    if (vertexShader == nullptr || fragmentShader == nullptr) {
        std::fprintf(stderr, "shader load failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GPUColorTargetDescription colorTarget{};
    colorTarget.format = SDL_GetGPUSwapchainTextureFormat(device, window);

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.vertex_shader = vertexShader;
    pipelineInfo.fragment_shader = fragmentShader;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipelineInfo.target_info.color_target_descriptions = &colorTarget;
    pipelineInfo.target_info.num_color_targets = 1;

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
    if (pipeline == nullptr) {
        std::fprintf(stderr, "pipeline creation failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_ReleaseGPUShader(device, vertexShader);
    SDL_ReleaseGPUShader(device, fragmentShader);

    TripleBuffer<SceneState> sceneBuffer;
    std::atomic<std::uint64_t> ticksProduced{0};
    std::jthread updateThread(updateLoop, std::ref(sceneBuffer), std::ref(ticksProduced));

    using clock = std::chrono::steady_clock;
    SceneState latest{};
    std::uint64_t lastTickSeen = 0;
    std::uint64_t frames = 0;
    std::uint64_t freshFrames = 0;
    std::uint64_t neverDrawn = 0;
    std::uint64_t ticksAtLastReport = 0;
    auto reportStart = clock::now();

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // Non-blocking poll: a zero timeout means "newest write, or nothing".
        if (auto read = sceneBuffer.GetForRead(0ms)) {
            latest = read.GetObject();
            neverDrawn += latest.Tick - lastTickSeen - 1;
            lastTickSeen = latest.Tick;
            ++freshFrames;
        }

        SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUTexture* swapchain = nullptr;
        Uint32 width = 0;
        Uint32 height = 0;
        // Blocks until a swapchain image is free, which paces us to vsync.
        if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, window, &swapchain, &width, &height)) {
            std::fprintf(stderr, "acquire failed: %s\n", SDL_GetError());
            SDL_SubmitGPUCommandBuffer(cmd);
            break;
        }

        if (swapchain != nullptr && height != 0) {
            SDL_GPUColorTargetInfo target{};
            target.texture = swapchain;
            target.clear_color = SDL_FColor{0.08F, 0.09F, 0.12F, 1.0F};
            target.load_op = SDL_GPU_LOADOP_CLEAR;
            target.store_op = SDL_GPU_STOREOP_STORE;

            const VertexParams params{.angle = latest.Angle,
                                      .aspect = static_cast<float>(width) / static_cast<float>(height),
                                      .padding = {0.0F, 0.0F}};

            SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &target, 1, nullptr);
            SDL_BindGPUGraphicsPipeline(pass, pipeline);
            SDL_PushGPUVertexUniformData(cmd, 0, &params, sizeof(params));
            SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
            SDL_EndGPURenderPass(pass);
        }
        SDL_SubmitGPUCommandBuffer(cmd);
        ++frames;

        if (clock::now() - reportStart >= 1s) {
            const std::uint64_t ticks = ticksProduced.load(std::memory_order_relaxed);
            std::printf("draw %llu fps | fresh frames %llu | update ticks %llu | updates never drawn %llu\n",
                        static_cast<unsigned long long>(frames),
                        static_cast<unsigned long long>(freshFrames),
                        static_cast<unsigned long long>(ticks - ticksAtLastReport),
                        static_cast<unsigned long long>(neverDrawn));
            std::fflush(stdout);
            frames = 0;
            freshFrames = 0;
            neverDrawn = 0;
            ticksAtLastReport = ticks;
            reportStart = clock::now();
        }
    }

    updateThread.request_stop();
    updateThread.join();

    SDL_WaitForGPUIdle(device);
    SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
