#include "panlid/graphics/rendering/sdl3/Sdl3GpuVertexBatch.hh"
#include "panlid/graphics/rendering/PrimitiveTopology.hh"
#include "panlid/graphics/rendering/sdl3/Sdl3GpuRenderer.hh"
#include "panlid/graphics/rendering/sdl3/Sdl3GpuVertexBuffer.hh"
#include "panlid/graphics/rendering/vertices/VertexMember.hh"
#include <cstddef>
#include <span>
#include <stdexcept>

namespace panlid::graphics::rendering::sdl3 {

Sdl3GpuVertexBatch::Sdl3GpuVertexBatch(Sdl3GpuRenderer& renderer,
                                       std::span<const vertices::VertexMember> layout,
                                       std::size_t vertexSize, int size,
                                       PrimitiveTopology primitiveType)
    : renderer_{renderer}, primitiveType_{primitiveType}, layout_{layout.begin(), layout.end()},
      vertexSize_{vertexSize}, size_{size} {}

void Sdl3GpuVertexBatch::AddVertex(std::span<const std::byte> vertex) {
    if (vertex.size() != vertexSize_) {
        throw std::invalid_argument("The vertex does not match the batch's vertex size.");
    }

    renderer_.SetActiveBatch(*this);

    VertexBufferList& buffers = currentVertexBuffers();

    if (!buffers.empty() && currentVertexIndex_ >= buffers[currentBufferIndex_]->GetSize()) {
        Draw();
        // todo: count the overflow in the frame statistics.

        ++currentBufferIndex_;
        currentVertexIndex_ = 0;
        currentDrawIndex_ = 0;
    }

    // The index changes after Draw() above, so this cannot be an else branch.
    if (currentBufferIndex_ >= buffers.size()) {
        buffers.push_back(Sdl3GpuVertexBuffer::Create(renderer_, size_, vertexSize_));
    }

    if (buffers[currentBufferIndex_]->SetVertex(currentVertexIndex_, vertex)) {
        if (synchronisationBeginIndex_ == -1) {
            synchronisationBeginIndex_ = currentVertexIndex_;
        }

        synchronisationEndIndex_ = currentVertexIndex_ + 1;
    }

    ++currentVertexIndex_;
}

int Sdl3GpuVertexBatch::Draw() {
    const int countToDraw = currentVertexIndex_ - currentDrawIndex_;
    const int drawStartIndex = currentDrawIndex_;
    currentDrawIndex_ = currentVertexIndex_;

    if (countToDraw == 0) {
        return 0;
    }

    Sdl3GpuVertexBuffer& buffer = *currentVertexBuffers()[currentBufferIndex_];

    if (synchronisationBeginIndex_ >= 0) {
        buffer.UpdateRange(synchronisationBeginIndex_, synchronisationEndIndex_);
    }

    renderer_.bindVertexBuffer(buffer.GetBuffer(), layout_, vertexSize_);
    renderer_.DrawVertices(primitiveType_, drawStartIndex, countToDraw);

    // Advancing to the next buffer with every draw would avoid contention with later vertex
    // updates.
    synchronisationBeginIndex_ = -1;

    return countToDraw;
}

Sdl3GpuVertexBatch::VertexBufferList& Sdl3GpuVertexBatch::currentVertexBuffers() {
    return vertexBuffers_[renderer_.GetFrameIndex() % vertexBuffers_.size()];
}

} // namespace panlid::graphics::rendering::sdl3
