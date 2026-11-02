#pragma once

#include "panlid/graphics/rendering/IVertexBuffer.hh"
#include <SDL3/SDL_gpu.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <stdexcept>
#include <vector>

namespace panlid::graphics::rendering::sdl3 {

class Sdl3GpuRenderer;

// A vertex buffer on an SDL GPU device. Vertices are written to CPU memory first, and the ranges
// that changed are copied to the GPU before drawing. Draw thread only.
//
// Destroy it before the renderer that owns the device it was created on.
//
// todo: track the buffer's native memory, once a memory tracker exists.
class Sdl3GpuVertexBuffer final : public IVertexBuffer,
                                  public std::enable_shared_from_this<Sdl3GpuVertexBuffer> {
  public:
    // size is the number of vertices, vertexSize the size of one vertex in bytes.
    [[nodiscard]] static std::shared_ptr<Sdl3GpuVertexBuffer>
    Create(Sdl3GpuRenderer& renderer, int size, std::size_t vertexSize);

    Sdl3GpuVertexBuffer(const Sdl3GpuVertexBuffer&) = delete;
    Sdl3GpuVertexBuffer& operator=(const Sdl3GpuVertexBuffer&) = delete;
    Sdl3GpuVertexBuffer(Sdl3GpuVertexBuffer&&) = delete;
    Sdl3GpuVertexBuffer& operator=(Sdl3GpuVertexBuffer&&) = delete;
    ~Sdl3GpuVertexBuffer() override;

    // The number of vertices in this buffer.
    [[nodiscard]] int GetSize() const { return size_; }

    // The GPU buffer to bind for drawing. Throws until the first UpdateRange.
    [[nodiscard]] SDL_GPUBuffer* GetBuffer() const {
        if (buffer_ == nullptr) {
            throw std::runtime_error("The buffer is not initialised yet.");
        }

        return buffer_;
    }

    // Sets the vertex at an index, given as its raw bytes. Returns whether the vertex changed, in
    // which case UpdateRange must cover it before drawing.
    bool SetVertex(int vertexIndex, std::span<const std::byte> vertex);

    // Copies the vertices in [startIndex, endIndex) to the GPU buffer.
    void UpdateRange(int startIndex, int endIndex);

    [[nodiscard]] std::uint64_t GetLastUseFrameIndex() const override { return lastUseFrameIndex_; }
    [[nodiscard]] bool IsInUse() const override { return lastUseFrameIndex_ > 0; }
    void Free() override { release(); }

  private:
    Sdl3GpuVertexBuffer(Sdl3GpuRenderer& renderer, int size, std::size_t vertexSize);

    Sdl3GpuRenderer& renderer_;
    SDL_GPUDevice* device_;
    int size_;
    std::size_t vertexSize_;
    std::vector<std::byte> staging_;
    SDL_GPUBuffer* buffer_ = nullptr;
    SDL_GPUTransferBuffer* transferBuffer_ = nullptr;
    int lastWrittenVertexIndex_ = -1;
    std::uint64_t lastUseFrameIndex_ = 0;

    [[nodiscard]] std::size_t byteOffset(int vertexIndex) const;
    [[nodiscard]] std::span<std::byte> stagingMemory();
    void initialiseGpuBuffer();
    void release() noexcept;
};

} // namespace panlid::graphics::rendering::sdl3
