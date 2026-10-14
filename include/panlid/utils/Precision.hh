#pragma once

#include "panlid/graphics/primitives/RectangleF.hh"
#include <cmath>
#include <glm/glm.hpp>

namespace panlid::utils {

inline constexpr float kFloatEpsilon = 1e-3F;
inline constexpr double kDoubleEpsilon = 1e-7;

[[nodiscard]] constexpr bool DefinitelyBigger(float value1, float value2,
                                              float acceptableDifference = kFloatEpsilon) {
    return value1 - acceptableDifference > value2;
}

[[nodiscard]] constexpr bool DefinitelyBigger(double value1, double value2,
                                              double acceptableDifference = kDoubleEpsilon) {
    return value1 - acceptableDifference > value2;
}

[[nodiscard]] constexpr bool AlmostBigger(float value1, float value2,
                                          float acceptableDifference = kFloatEpsilon) {
    return value1 > value2 - acceptableDifference;
}

[[nodiscard]] constexpr bool AlmostBigger(double value1, double value2,
                                          double acceptableDifference = kDoubleEpsilon) {
    return value1 > value2 - acceptableDifference;
}

[[nodiscard]] inline bool AlmostEquals(float value1, float value2,
                                       float acceptableDifference = kFloatEpsilon) {
    return std::abs(value1 - value2) <= acceptableDifference;
}

[[nodiscard]] inline bool AlmostEquals(const glm::vec2& value1, const glm::vec2& value2,
                                       float acceptableDifference = kFloatEpsilon) {
    return AlmostEquals(value1.x, value2.x, acceptableDifference) &&
           AlmostEquals(value1.y, value2.y, acceptableDifference);
}

[[nodiscard]] inline bool AlmostEquals(double value1, double value2,
                                       double acceptableDifference = kDoubleEpsilon) {
    return std::abs(value1 - value2) <= acceptableDifference;
}

[[nodiscard]] constexpr bool AlmostIntersects(const graphics::primitives::RectangleF& rect1,
                                              const graphics::primitives::RectangleF& rect2,
                                              float acceptableDifference = kFloatEpsilon) {
    return rect1.X <= rect2.X + rect2.Width + acceptableDifference &&
           rect2.X <= rect1.X + rect1.Width + acceptableDifference &&
           rect1.Y <= rect2.Y + rect2.Height + acceptableDifference &&
           rect2.Y <= rect1.Y + rect1.Height + acceptableDifference;
}

} // namespace panlid::utils
