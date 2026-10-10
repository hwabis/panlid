#pragma once

#include "panlid/graphics/rendering/BufferTestFunction.hh"
#include "panlid/graphics/rendering/StencilOperation.hh"

namespace panlid::graphics::rendering {

class StencilInfo {
  public:
    /// The default stencil properties.
    [[nodiscard]] static constexpr StencilInfo GetDefault() { return StencilInfo{false}; }

    /// Whether stencil testing should occur.
    /// If this is false, no StencilOperation will occur (use BufferTestFunction::Always instead).
    [[nodiscard]] constexpr bool IsStencilTest() const { return stencilTest_; }
    /// The stencil test function.
    [[nodiscard]] constexpr BufferTestFunction GetTestFunction() const { return testFunction_; }
    /// The stencil test value to compare against.
    [[nodiscard]] constexpr int GetTestValue() const { return testValue_; }
    /// The stencil mask.
    [[nodiscard]] constexpr int GetMask() const { return mask_; }
    /// The operation to perform on the stencil buffer when the stencil test fails.
    [[nodiscard]] constexpr StencilOperation GetStencilTestFailOperation() const {
        return stencilFailed_;
    }
    /// The operation to perform on the stencil buffer when the depth test fails.
    [[nodiscard]] constexpr StencilOperation GetDepthTestFailOperation() const {
        return depthFailed_;
    }
    /// The operation to perform on the stencil buffer when both the stencil and depth tests pass.
    [[nodiscard]] constexpr StencilOperation GetTestPassedOperation() const { return passed_; }

    explicit constexpr StencilInfo(bool stencilTest = true,
                                   BufferTestFunction testFunction = BufferTestFunction::Always,
                                   int testValue = 1, int mask = 0xff,
                                   StencilOperation stencilFailed = StencilOperation::Keep,
                                   StencilOperation depthFailed = StencilOperation::Keep,
                                   StencilOperation passed = StencilOperation::Replace)
        : stencilTest_(stencilTest), testFunction_(testFunction), testValue_(testValue),
          mask_(mask), stencilFailed_(stencilFailed), depthFailed_(depthFailed), passed_(passed) {}

    constexpr bool operator==(const StencilInfo& other) const = default;

  private:
    bool stencilTest_;
    BufferTestFunction testFunction_;
    int testValue_;
    int mask_;
    StencilOperation stencilFailed_;
    StencilOperation depthFailed_;
    StencilOperation passed_;
};

} // namespace panlid::graphics::rendering
