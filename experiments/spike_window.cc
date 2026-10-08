#include <SDL3/SDL.h>

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#ifndef SPIKE_SHADER_DIR
#define SPIKE_SHADER_DIR "."
#endif

namespace {

std::vector<Uint8> readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

SDL_GPUShader* loadShader(SDL_GPUDevice* device, const std::string& file,
                          SDL_GPUShaderStage stage) {
    const std::vector<Uint8> code = readFile(std::string(SPIKE_SHADER_DIR) + "/" + file);
    if (code.empty()) {
        std::fprintf(stderr, "could not read shader %s\n", file.c_str());
        return nullptr;
    }

    SDL_GPUShaderCreateInfo info{};
    info.code = code.data();
    info.code_size = code.size();
    info.entrypoint = "main";
    info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    info.stage = stage;
    return SDL_CreateGPUShader(device, &info);
}

} // namespace

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("panlid spike", 1280, 720, SDL_WINDOW_RESIZABLE);
    SDL_GPUDevice* device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, nullptr);
    if (window == nullptr || device == nullptr) {
        std::fprintf(stderr, "window/device creation failed: %s\n", SDL_GetError());
        return 1;
    }
    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        std::fprintf(stderr, "claim window failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GPUShader* vertexShader = loadShader(device, "tri.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX);
    SDL_GPUShader* fragmentShader =
        loadShader(device, "tri.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT);
    if (vertexShader == nullptr || fragmentShader == nullptr) {
        std::fprintf(stderr, "shader creation failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GPUColorTargetDescription colourTarget{};
    colourTarget.format = SDL_GetGPUSwapchainTextureFormat(device, window);

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.vertex_shader = vertexShader;
    pipelineInfo.fragment_shader = fragmentShader;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipelineInfo.target_info.color_target_descriptions = &colourTarget;
    pipelineInfo.target_info.num_color_targets = 1;

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
    SDL_ReleaseGPUShader(device, vertexShader);
    SDL_ReleaseGPUShader(device, fragmentShader);
    if (pipeline == nullptr) {
        std::fprintf(stderr, "pipeline creation failed: %s\n", SDL_GetError());
        return 1;
    }

    bool running = true;
    while (running) {
        for (SDL_Event event; SDL_PollEvent(&event);) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUTexture* swapchain = nullptr;
        if (SDL_WaitAndAcquireGPUSwapchainTexture(cmd, window, &swapchain, nullptr, nullptr) &&
            swapchain != nullptr) {
            SDL_GPUColorTargetInfo target{};
            target.texture = swapchain;
            target.clear_color = {0.1F, 0.2F, 0.4F, 1.0F};
            target.load_op = SDL_GPU_LOADOP_CLEAR;
            target.store_op = SDL_GPU_STOREOP_STORE;

            SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &target, 1, nullptr);
            SDL_BindGPUGraphicsPipeline(pass, pipeline);
            SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
            SDL_EndGPURenderPass(pass);
        }
        SDL_SubmitGPUCommandBuffer(cmd);
    }

    SDL_WaitForGPUIdle(device);
    SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
