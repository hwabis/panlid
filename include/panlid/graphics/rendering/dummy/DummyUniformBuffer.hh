#pragma once

#include "panlid/graphics/rendering/IUniformBuffer.hh"
#include <cstddef>
#include <span>

namespace panlid::graphics::rendering::dummy {

// A uniform buffer that discards its data.
class DummyUniformBuffer final : public IUniformBuffer {
  public:
    void SetData(std::span<const std::byte> /*data*/) override {}
};

} // namespace panlid::graphics::rendering::dummy
