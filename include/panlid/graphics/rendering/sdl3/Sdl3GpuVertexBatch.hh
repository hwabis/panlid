#pragma once

#include "panlid/graphics/rendering/IVertexBatch.hh"
#include "panlid/graphics/rendering/PrimitiveTopology.hh"
#include "panlid/graphics/rendering/sdl3/Sdl3GpuVertexBuffer.hh"
#include "panlid/graphics/rendering/vertices/VertexMember.hh"
#include <array>
#include <cstddef>
#include <memory>
#include <span>
#include <vector>

namespace panlid::graphics::rendering::sdl3 {

class Sdl3GpuRenderer;

// Collects vertices and draws them through a ring of vertex buffers, so the CPU never writes a
// buffer the GPU may still be reading. Draw thread only.
//
// Destroy it before the renderer it was created with.
//
// todo: index layouts and the quad batch, once the index buffer is ported. Both are this class with
// different constructor arguments.
class Sdl3GpuVertexBatch final : public IVertexBatch {
  public:
    // size is the number of vertices in each buffer. The layout is copied.
    Sdl3GpuVertexBatch(Sdl3GpuRenderer& renderer, std::span<const vertices::VertexMember> layout,
                       std::size_t vertexSize, int size, PrimitiveTopology primitiveType);

    [[nodiscard]] int GetSize() const override { return size_; }

    int Draw() override;

    void ResetCounters() override {
        synchronisationBeginIndex_ = -1;
        currentBufferIndex_ = 0;
        currentVertexIndex_ = 0;
        currentDrawIndex_ = 0;
    }

    void AddVertex(std::span<const std::byte> vertex) override;

  private:
    // Three buffers avoid contention. The draw nodes already sit behind a triple buffer, and the
    // renderer's default batch is shared, so two would be enough in theory.
    // todo: allow overriding the count.
    static constexpr std::size_t kVertexBufferCount = 3;

    using VertexBufferList = std::vector<std::shared_ptr<Sdl3GpuVertexBuffer>>;

    Sdl3GpuRenderer& renderer_;
    PrimitiveTopology primitiveType_;
    std::vector<vertices::VertexMember> layout_;
    std::size_t vertexSize_;
    int size_;

    std::array<VertexBufferList, kVertexBufferCount> vertexBuffers_;

    // The range of vertices that must be copied to the GPU before the next draw.
    int synchronisationBeginIndex_ = -1;
    int synchronisationEndIndex_ = -1;

    std::size_t currentBufferIndex_ = 0;
    int currentVertexIndex_ = 0;
    int currentDrawIndex_ = 0;

    [[nodiscard]] VertexBufferList& currentVertexBuffers();
};

} // namespace panlid::graphics::rendering::sdl3
