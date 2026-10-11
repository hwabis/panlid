#pragma once

#include <concepts>

namespace panlid::graphics::transforms {

template <typename T>
concept EasingFunction = requires(const T& easing, double time) {
    { easing.ApplyEasing(time) } -> std::same_as<double>;
};

} // namespace panlid::graphics::transforms
