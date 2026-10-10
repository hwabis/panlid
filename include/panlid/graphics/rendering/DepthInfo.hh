#pragma once

#include "panlid/graphics/rendering/BufferTestFunction.hh"

namespace panlid::graphics::rendering {

class DepthInfo {
  public:
    /// The default depth properties, as defined by OpenGL.
    [[nodiscard]] static constexpr DepthInfo GetDefault() { return DepthInfo{true}; }

    /// Whether depth testing should occur.
    [[nodiscard]] constexpr bool IsDepthTest() const { return depthTest_; }
    /// Whether to write to the depth buffer if the depth test passed.
    [[nodiscard]] constexpr bool IsWriteDepth() const { return writeDepth_; }
    /// The depth test function.
    [[nodiscard]] constexpr BufferTestFunction GetFunction() const { return function_; }

    explicit constexpr DepthInfo(bool depthTest = true, bool writeDepth = true,
                                 BufferTestFunction function = BufferTestFunction::LessThan)
        : depthTest_(depthTest), writeDepth_(writeDepth), function_(function) {}

    constexpr bool operator==(const DepthInfo& other) const = default;

  private:
    bool depthTest_;
    bool writeDepth_;
    BufferTestFunction function_;
};

} // namespace panlid::graphics::rendering
