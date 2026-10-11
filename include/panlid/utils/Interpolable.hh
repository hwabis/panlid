#pragma once

#include "panlid/graphics/transforms/DefaultEasingFunction.hh"
#include <concepts>

namespace panlid::utils {

template <typename T>
concept Interpolable =
    requires(double time, const T& start, const T& end, double startTime, double endTime,
             const graphics::transforms::DefaultEasingFunction& easing) {
        { T::ValueAt(time, start, end, startTime, endTime, easing) } -> std::same_as<T>;
    };

} // namespace panlid::utils
