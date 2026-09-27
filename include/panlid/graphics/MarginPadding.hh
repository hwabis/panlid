#pragma once

#include "panlid/graphics/transforms/EasingFunction.hh"
#include "panlid/utils/Interpolable.hh"
#include "panlid/utils/Interpolation.hh"
#include <glm/vec2.hpp>

namespace panlid::graphics {

struct MarginPadding {
    float Top{};
    float Left{};
    float Bottom{};
    float Right{};

    constexpr MarginPadding() = default;
    constexpr explicit MarginPadding(float allSides)
        : Top(allSides), Left(allSides), Bottom(allSides), Right(allSides) {}

    [[nodiscard]] constexpr float TotalHorizontal() const { return Left + Right; }
    constexpr void SetHorizontal(float value) { Left = Right = value; }

    [[nodiscard]] constexpr float TotalVertical() const { return Top + Bottom; }
    constexpr void SetVertical(float value) { Top = Bottom = value; }

    [[nodiscard]] constexpr glm::vec2 Total() const { return {TotalHorizontal(), TotalVertical()}; }

    constexpr bool operator==(const MarginPadding& other) const = default;

    constexpr MarginPadding operator-() const {
        MarginPadding result;
        result.Left = -Left;
        result.Top = -Top;
        result.Right = -Right;
        result.Bottom = -Bottom;
        return result;
    }

    constexpr MarginPadding operator*(glm::vec2 v) const {
        MarginPadding result;
        result.Left = Left * v.x;
        result.Top = Top * v.y;
        result.Right = Right * v.x;
        result.Bottom = Bottom * v.y;
        return result;
    }
};

template <graphics::transforms::EasingFunction TEasing>
[[nodiscard]] MarginPadding ValueAt(double time, const MarginPadding& startValue,
                                    const MarginPadding& endValue, double startTime, double endTime,
                                    const TEasing& easing) {
    MarginPadding result;
    result.Left = utils::ValueAt(time, startValue.Left, endValue.Left, startTime, endTime, easing);
    result.Top = utils::ValueAt(time, startValue.Top, endValue.Top, startTime, endTime, easing);
    result.Right =
        utils::ValueAt(time, startValue.Right, endValue.Right, startTime, endTime, easing);
    result.Bottom =
        utils::ValueAt(time, startValue.Bottom, endValue.Bottom, startTime, endTime, easing);
    return result;
}

static_assert(utils::Interpolable<MarginPadding>, "MarginPadding must satisfy Interpolable");

} // namespace panlid::graphics
