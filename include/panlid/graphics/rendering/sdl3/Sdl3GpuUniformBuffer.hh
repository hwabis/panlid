#pragma once

#include "panlid/graphics/rendering/IUniformBuffer.hh"
#include <algorithm>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <vector>

namespace panlid::graphics::rendering::sdl3 {

// Holds the data of a uniform block. The pipeline pushes it to the command buffer when it draws.
class Sdl3GpuUniformBuffer final : public IUniformBuffer {
  public:
    explicit Sdl3GpuUniformBuffer(std::size_t size) : data_(size) {}

    void SetData(std::span<const std::byte> data) override {
        if (data.size() != data_.size()) {
            throw std::invalid_argument("Uniform data must be exactly as large as the buffer.");
        }

        std::ranges::copy(data, data_.begin());
    }

    [[nodiscard]] std::span<const std::byte> GetData() const { return data_; }

  private:
    std::vector<std::byte> data_;
};

} // namespace panlid::graphics::rendering::sdl3
