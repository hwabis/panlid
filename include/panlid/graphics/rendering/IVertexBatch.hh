#pragma once

#include <cstddef>
#include <span>

namespace panlid::graphics::rendering {

class IVertexBatch {
  public:
    IVertexBatch() = default;
    IVertexBatch(const IVertexBatch&) = delete;
    IVertexBatch(IVertexBatch&&) = delete;
    IVertexBatch& operator=(const IVertexBatch&) = delete;
    IVertexBatch& operator=(IVertexBatch&&) = delete;
    virtual ~IVertexBatch() = default;

    // The number of vertices in each vertex buffer.
    [[nodiscard]] virtual int GetSize() const = 0;

    // Draws any newly added indices in this batch. Returns the number of indices drawn, if any.
    virtual int Draw() = 0;

    virtual void ResetCounters() = 0;

    // Adds one vertex, given as the raw bytes of a vertex of the layout this batch was created
    // with. todo: typed wrapper (Add(const T&) and the cached AddAction) once DrawNodes need it.
    virtual void AddVertex(std::span<const std::byte> vertex) = 0;
};

} // namespace panlid::graphics::rendering
