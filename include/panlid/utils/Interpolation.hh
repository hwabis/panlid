#pragma once

#include "panlid/graphics/Easing.hh"
#include "panlid/graphics/transforms/DefaultEasingFunction.hh"
#include "panlid/graphics/transforms/EasingFunction.hh"
#include "panlid/utils/Interpolable.hh"
#include <cmath>
#include <concepts>
#include <cstddef>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <span>
#include <stdexcept>
#include <vector>

namespace panlid::utils {

template <typename T>
concept EasingFunction = graphics::transforms::EasingFunction<T>;

[[nodiscard]] constexpr double Lerp(double start, double final, double amount) {
    return start + (final - start) * amount;
}

[[nodiscard]] inline double Damp(double start, double final, double base, double exponent) {
    if (base < 0 || base > 1) {
        throw std::invalid_argument("base has to lie in [0,1].");
    }
    return Lerp(start, final, 1 - std::pow(base, exponent));
}

[[nodiscard]] inline double DampContinuously(double current, double target, double halfTime,
                                             double elapsedTime) {
    double exponent = elapsedTime / halfTime;
    return Damp(current, target, 0.5, exponent);
}

[[nodiscard]] inline double LagrangeBasis(std::span<const glm::vec2> points, int base,
                                          double time) {
    double product = 1;
    for (int i = 0; i < static_cast<int>(points.size()); ++i) {
        if (i != base) {
            product *= (time - points[i].x) / (points[base].x - points[i].x);
        }
    }
    return product;
}

[[nodiscard]] inline double Lagrange(std::span<const glm::vec2> points, double time) {
    if (points.empty()) {
        throw std::invalid_argument("points must contain at least one point");
    }
    double sum = 0;
    for (int i = 0; i < static_cast<int>(points.size()); ++i) {
        sum += points[i].y * LagrangeBasis(points, i, time);
    }
    return sum;
}

[[nodiscard]] inline std::vector<double> BarycentricWeights(std::span<const glm::vec2> points) {
    std::vector<double> w(points.size(), 1.0);
    for (std::size_t i = 0; i < points.size(); ++i) {
        for (std::size_t j = 0; j < points.size(); ++j) {
            if (i != j) {
                w[i] *= points[i].x - points[j].x;
            }
        }
        w[i] = 1.0 / w[i];
    }
    return w;
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
        if (time == points[i].x) {
            return points[i].y;
        }

        double li = weights[i] / (time - points[i].x);
        numerator += li * points[i].y;
        denominator += li;
    }

    return numerator / denominator;
}

template <typename TValue, EasingFunction TEasing>
    requires std::is_integral_v<TValue>
[[nodiscard]] TValue ValueAt(double time, TValue val1, TValue val2, double startTime,
                             double endTime, const TEasing& easing) {
    if (val1 == val2) {
        return val1;
    }

    double current = time - startTime;
    double duration = endTime - startTime;

    if (current == 0) {
        return val1;
    }
    if (duration == 0) {
        return val2;
    }

    double t = easing.ApplyEasing(current / duration);
    return static_cast<TValue>(std::nearbyint(
        static_cast<double>(val1) + t * (static_cast<double>(val2) - static_cast<double>(val1))));
}

template <std::floating_point TValue, EasingFunction TEasing>
[[nodiscard]] TValue ValueAt(double time, TValue val1, TValue val2, double startTime,
                             double endTime, const TEasing& easing) {
    if (val1 == val2) {
        return val1;
    }
    const double current = time - startTime;
    const double duration = endTime - startTime;
    if (current == 0) {
        return val1;
    }
    if (duration == 0) {
        return val2;
    }
    const double t = easing.ApplyEasing(current / duration);
    return static_cast<TValue>(static_cast<double>(val1) +
                               t * (static_cast<double>(val2) - static_cast<double>(val1)));
}

template <typename T>
concept Lerpable = !std::is_arithmetic_v<T> && requires(const T& a, const T& b, float t) {
    { a - b } -> std::convertible_to<T>;
    { t * (a - b) } -> std::convertible_to<T>;
    { a + t * (a - b) } -> std::convertible_to<T>;
};

template <Lerpable TValue, EasingFunction TEasing>
[[nodiscard]] TValue ValueAt(double time, TValue val1, TValue val2, double startTime,
                             double endTime, const TEasing& easing) {
    auto current = static_cast<float>(time - startTime);
    auto duration = static_cast<float>(endTime - startTime);

    if (duration == 0 || current == 0) {
        return val1;
    }

    auto t = static_cast<float>(easing.ApplyEasing(current / duration));
    return val1 + t * (val2 - val1);
}

template <Interpolable TValue, EasingFunction TEasing>
[[nodiscard]] TValue ValueAt(double time, const TValue& startValue, const TValue& endValue,
                             double startTime, double endTime, const TEasing& easing) {
    return TValue::ValueAt(time, startValue, endValue, startTime, endTime, easing);
}

template <typename TValue>
[[nodiscard]] TValue ValueAt(double time, TValue val1, TValue val2, double startTime,
                             double endTime, graphics::Easing easing = graphics::Easing::None) {
    return ValueAt(time, val1, val2, startTime, endTime,
                   graphics::transforms::DefaultEasingFunction(easing));
}

template <EasingFunction TEasing>
[[nodiscard]] double ApplyEasing(const TEasing& easing, double time) {
    return easing.ApplyEasing(time);
}

[[nodiscard]] inline double ApplyEasing(graphics::Easing easing, double time) {
    return ApplyEasing(graphics::transforms::DefaultEasingFunction(easing), time);
}

} // namespace panlid::utils
