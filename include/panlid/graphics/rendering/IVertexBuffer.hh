#pragma once

#include <cstdint>

namespace panlid::graphics::rendering {

// Internal interface for the vertex buffers a Renderer frees when they go unused.
class IVertexBuffer {
  public:
    IVertexBuffer() = default;
    IVertexBuffer(const IVertexBuffer&) = delete;
    IVertexBuffer(IVertexBuffer&&) = delete;
    IVertexBuffer& operator=(const IVertexBuffer&) = delete;
    IVertexBuffer& operator=(IVertexBuffer&&) = delete;
    virtual ~IVertexBuffer() = default;

    // The Renderer frame index when this buffer was last used.
    [[nodiscard]] virtual uint64_t GetLastUseFrameIndex() const = 0;

    [[nodiscard]] virtual bool IsInUse() const = 0;

    // Frees all resources allocated by this buffer. Afterwards IsInUse() must be false.
    virtual void Free() = 0;
};

} // namespace panlid::graphics::rendering
