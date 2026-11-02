#include "panlid/graphics/rendering/sdl3/Sdl3GpuVertexBuffer.hh"
#include "panlid/development/ThreadSafety.hh"
#include "panlid/graphics/rendering/sdl3/Sdl3GpuRenderer.hh"
#include "panlid/platform/sdl3/Sdl3Exception.hh"
#include <SDL3/SDL_gpu.h>
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <stdexcept>

namespace panlid::graphics::rendering::sdl3 {

std::shared_ptr<Sdl3GpuVertexBuffer> Sdl3GpuVertexBuffer::Create(Sdl3GpuRenderer& renderer,
                                                                 int size, std::size_t vertexSize) {
    return std::shared_ptr<Sdl3GpuVertexBuffer>(
        new Sdl3GpuVertexBuffer(renderer, size, vertexSize));
}

Sdl3GpuVertexBuffer::Sdl3GpuVertexBuffer(Sdl3GpuRenderer& renderer, int size,
                                         std::size_t vertexSize)
    : renderer_{renderer}, device_{renderer.GetDevice()}, size_{size}, vertexSize_{vertexSize} {
    if (size <= 0 || vertexSize == 0) {
        throw std::invalid_argument("A vertex buffer needs a positive size and vertex size.");
    }
}

Sdl3GpuVertexBuffer::~Sdl3GpuVertexBuffer() { release(); }

bool Sdl3GpuVertexBuffer::SetVertex(int vertexIndex, std::span<const std::byte> vertex) {
    assert(vertexIndex >= 0 && vertexIndex < size_);
    assert(vertex.size() == vertexSize_);

    const std::span<std::byte> current =
        stagingMemory().subspan(byteOffset(vertexIndex), vertexSize_);
    const bool isNewVertex =
        vertexIndex > lastWrittenVertexIndex_ || !std::ranges::equal(current, vertex);

    std::ranges::copy(vertex, current.begin());
    lastWrittenVertexIndex_ = std::max(lastWrittenVertexIndex_, vertexIndex);

    return isNewVertex;
}

void Sdl3GpuVertexBuffer::UpdateRange(int startIndex, int endIndex) {
    assert(startIndex >= 0 && startIndex <= endIndex && endIndex <= size_);

    if (buffer_ == nullptr) {
        initialiseGpuBuffer();
    }

    SDL_GPUCommandBuffer* commandBuffer = renderer_.currentCommandBuffer();

    if (commandBuffer == nullptr) {
        throw std::runtime_error("Cannot upload vertices outside of a frame.");
    }

    const std::size_t offset = byteOffset(startIndex);
    const std::size_t count = byteOffset(endIndex) - offset;

    // Cycling gives the transfer buffer fresh memory when an earlier upload still reads the old
    // memory. Only the range written here is read back, so nothing else is lost.
    void* mapped = SDL_MapGPUTransferBuffer(device_, transferBuffer_, true);

    if (mapped == nullptr) {
        throw platform::sdl3::Sdl3Exception{};
    }

    const std::span<std::byte> mappedMemory{static_cast<std::byte*>(mapped), staging_.size()};
    std::ranges::copy(std::span<const std::byte>{staging_}.subspan(offset, count),
                      mappedMemory.subspan(offset, count).begin());
    SDL_UnmapGPUTransferBuffer(device_, transferBuffer_);

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);

    if (copyPass == nullptr) {
        throw platform::sdl3::Sdl3Exception{};
    }

    const SDL_GPUTransferBufferLocation source{.transfer_buffer = transferBuffer_,
                                               .offset = static_cast<std::uint32_t>(offset)};
    const SDL_GPUBufferRegion target{.buffer = buffer_,
                                     .offset = static_cast<std::uint32_t>(offset),
                                     .size = static_cast<std::uint32_t>(count)};

    // The buffer is not cycled, because only part of it is written and the rest must survive.
    SDL_UploadToGPUBuffer(copyPass, &source, &target, false);
    SDL_EndGPUCopyPass(copyPass);

    // todo: count the uploaded vertices in the frame statistics.
}

std::size_t Sdl3GpuVertexBuffer::byteOffset(int vertexIndex) const {
    return static_cast<std::size_t>(vertexIndex) * vertexSize_;
}

std::span<std::byte> Sdl3GpuVertexBuffer::stagingMemory() {
    assert(development::ThreadSafety::IsDrawThread);

    if (!IsInUse()) {
        staging_.assign(byteOffset(size_), std::byte{});
        renderer_.RegisterVertexBufferUse(shared_from_this());
    }

    lastUseFrameIndex_ = renderer_.GetFrameIndex();

    return staging_;
}

void Sdl3GpuVertexBuffer::initialiseGpuBuffer() {
    assert(development::ThreadSafety::IsDrawThread);
    assert(!staging_.empty());

    SDL_GPUBufferCreateInfo bufferInfo{};
    bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    bufferInfo.size = static_cast<std::uint32_t>(staging_.size());

    buffer_ = SDL_CreateGPUBuffer(device_, &bufferInfo);

    if (buffer_ == nullptr) {
        throw platform::sdl3::Sdl3Exception{};
    }

    SDL_GPUTransferBufferCreateInfo transferInfo{};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = static_cast<std::uint32_t>(staging_.size());

    transferBuffer_ = SDL_CreateGPUTransferBuffer(device_, &transferInfo);

    if (transferBuffer_ == nullptr) {
        throw platform::sdl3::Sdl3Exception{};
    }
}

void Sdl3GpuVertexBuffer::release() noexcept {
    if (transferBuffer_ != nullptr) {
        SDL_ReleaseGPUTransferBuffer(device_, transferBuffer_);
        transferBuffer_ = nullptr;
    }

    if (buffer_ != nullptr) {
        SDL_ReleaseGPUBuffer(device_, buffer_);
        buffer_ = nullptr;
    }

    staging_.clear();
    staging_.shrink_to_fit();

    // A recreated buffer starts empty, so every vertex must count as new again. Otherwise a vertex
    // equal to the zeroed memory would never be uploaded.
    lastWrittenVertexIndex_ = -1;
    lastUseFrameIndex_ = 0;
}

} // namespace panlid::graphics::rendering::sdl3
