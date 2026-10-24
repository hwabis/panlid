#pragma once

#include "panlid/graphics/rendering/IVertexBatch.hh"
#include <cstddef>
#include <span>

namespace panlid::graphics::rendering::dummy {

// A vertex batch that discards everything added to it.
class DummyVertexBatch final : public IVertexBatch {
  public:
    [[nodiscard]] int GetSize() const override { return 0; }

    int Draw() override { return 0; }

    void ResetCounters() override {}

    void AddVertex(std::span<const std::byte> /*vertex*/) override {}
};

} // namespace panlid::graphics::rendering::dummy
