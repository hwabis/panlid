#pragma once

#include "panlid/graphics/transforms/DefaultEasingFunction.hh"
#include <concepts>

namespace panlid::utils {

template <typename T>
concept Interpolable = requires(const T& value, double time,
                                const graphics::transforms::DefaultEasingFunction& easing) {
    { value.ValueAt(time, value, value, time, time, easing) } -> std::same_as<T>;
};

} // namespace panlid::utils
