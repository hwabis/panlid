#pragma once

#include "panlid/graphics/Easing.hh"
#include "panlid/graphics/transforms/DefaultEasingFunction.hh"
#include "panlid/graphics/transforms/EasingFunction.hh"
#include "panlid/utils/Interpolable.hh"
#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <format>
#include <glm/glm.hpp>
#include <limits>
#include <span>
#include <stdexcept>
#include <vector>

namespace panlid::utils {

[[nodiscard]] constexpr double Lerp(double start, double end, double amount) {
    return start + (end - start) * amount;
}

// base must lie in [0, 1]: smaller converges faster. exponent 0 gives start.
[[nodiscard]] inline double Damp(double start, double end, double base, double exponent) {
    if (base < 0 || base > 1) {
        throw std::invalid_argument(std::format("base has to lie in [0,1], but is {}.", base));
    }

    return Lerp(start, end, 1 - std::pow(base, exponent));
}

// Approximately frame-rate independent when called once per frame, but floating-point error
// accumulates, so don't use it where exact values matter. halfTime is the time to cover half of
// the remaining distance.
[[nodiscard]] inline double DampContinuously(double current, double target, double halfTime,
                                             double elapsedTime) {
    const double exponent = elapsedTime / halfTime;
    return Damp(current, target, 0.5, exponent);
}

[[nodiscard]] inline double LagrangeBasis(std::span<const glm::vec2> points, std::size_t base,
                                          double time) {
    double product = 1;

    for (std::size_t i = 0; i < points.size(); ++i) {
        if (i != base) {
            product *= (time - points[i].x) / (points[base].x - points[i].x);
        }
    }

    return product;
}

// points must not contain two equal x values.
[[nodiscard]] inline double Lagrange(std::span<const glm::vec2> points, double time) {
    if (points.empty()) {
        throw std::invalid_argument("points must contain at least one point");
    }

    double sum = 0;
    for (std::size_t i = 0; i < points.size(); ++i) {
        sum += points[i].y * LagrangeBasis(points, i, time);
    }

    return sum;
}

// points must not contain two equal x values.
[[nodiscard]] inline std::vector<double> BarycentricWeights(std::span<const glm::vec2> points) {
    std::vector<double> weights(points.size());

    for (std::size_t i = 0; i < points.size(); ++i) {
        double weight = 1;

        for (std::size_t j = 0; j < points.size(); ++j) {
            if (i != j) {
                weight *= points[i].x - points[j].x;
            }
        }

        weights[i] = 1.0 / weight;
    }

    return weights;
}

[[nodiscard]] inline double BarycentricLagrange(std::span<const glm::vec2> points,
                                                std::span<const double> weights, double time) {
    if (points.empty()) {
        throw std::invalid_argument("points must contain at least one point");
    }

    if (points.size() != weights.size()) {
        throw std::invalid_argument("points must contain exactly as many items as weights");
    }

    double numerator = 0;
    double denominator = 0;

    for (std::size_t i = 0; i < points.size(); ++i) {
        // Exact comparison on purpose: it prevents NaN at control point x coordinates.
        if (time == points[i].x) {
            return points[i].y;
        }

        const double li = weights[i] / (time - points[i].x);
        numerator += li * points[i].y;
        denominator += li;
    }

    return numerator / denominator;
}

[[nodiscard]] inline double ApplyEasing(graphics::Easing easing, double time) {
    return graphics::transforms::DefaultEasingFunction(easing).ApplyEasing(time);
}

template <graphics::transforms::EasingFunction E>
[[nodiscard]] double ValueAt(double time, double start, double end, double startTime,
                             double endTime, const E& easing) {
    if (start == end) {
        return start;
    }

    const double current = time - startTime;
    const double duration = endTime - startTime;

    if (current == 0) {
        return start;
    }
    if (duration == 0) {
        return end;
    }

    const double t = easing.ApplyEasing(current / duration);
    return start + t * (end - start);
}

template <graphics::transforms::EasingFunction E>
[[nodiscard]] float ValueAt(double time, float start, float end, double startTime, double endTime,
                            const E& easing) {
    return static_cast<float>(ValueAt(time, static_cast<double>(start), static_cast<double>(end),
                                      startTime, endTime, easing));
}

template <std::integral T, graphics::transforms::EasingFunction E>
[[nodiscard]] T ValueAt(double time, T start, T end, double startTime, double endTime,
                        const E& easing)
    requires(!std::same_as<T, bool> && sizeof(T) <= sizeof(std::int32_t))
{
    const double value = std::nearbyint(ValueAt(
        time, static_cast<double>(start), static_cast<double>(end), startTime, endTime, easing));

    // Easings like Back and Elastic overshoot, and an out-of-range double-to-int cast is UB.
    return static_cast<T>(std::clamp(value, static_cast<double>(std::numeric_limits<T>::min()),
                                     static_cast<double>(std::numeric_limits<T>::max())));
}

template <typename V, graphics::transforms::EasingFunction E>
[[nodiscard]] V ValueAt(double time, V start, V end, double startTime, double endTime,
                        const E& easing)
    requires std::same_as<V, glm::vec2> || std::same_as<V, glm::vec3>
{
    const auto current = static_cast<float>(time - startTime);
    const auto duration = static_cast<float>(endTime - startTime);

    if (duration == 0 || current == 0) {
        return start;
    }

    const auto t = static_cast<float>(easing.ApplyEasing(current / duration));
    return start + t * (end - start);
}

template <Interpolable T, graphics::transforms::EasingFunction E>
[[nodiscard]] T ValueAt(double time, const T& start, const T& end, double startTime, double endTime,
                        const E& easing) {
    return start.ValueAt(time, start, end, startTime, endTime, easing);
}

template <typename T>
[[nodiscard]] T ValueAt(double time, const T& start, const T& end, double startTime, double endTime,
                        graphics::Easing easing = graphics::Easing::None) {
    return ValueAt(time, start, end, startTime, endTime,
                   graphics::transforms::DefaultEasingFunction(easing));
}

} // namespace panlid::utils
