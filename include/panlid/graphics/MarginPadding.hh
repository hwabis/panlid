#pragma once

#include "panlid/graphics/transforms/EasingFunction.hh"
#include "panlid/utils/Interpolable.hh"
#include "panlid/utils/Interpolation.hh"
#include <glm/glm.hpp>

namespace panlid::graphics {

struct MarginPadding {
    float Top{};
    float Left{};
    float Bottom{};
    float Right{};

    [[nodiscard]] static constexpr MarginPadding FromAllSides(float allSides) {
        return {.Top = allSides, .Left = allSides, .Bottom = allSides, .Right = allSides};
    }

    [[nodiscard]] constexpr float GetTotalHorizontal() const { return Left + Right; }

    constexpr void SetHorizontal(float value) { Left = Right = value; }

    [[nodiscard]] constexpr float GetTotalVertical() const { return Top + Bottom; }

    constexpr void SetVertical(float value) { Top = Bottom = value; }

    [[nodiscard]] constexpr glm::vec2 GetTotal() const {
        return {GetTotalHorizontal(), GetTotalVertical()};
    }

    [[nodiscard]] bool operator==(const MarginPadding&) const = default;

    [[nodiscard]] constexpr MarginPadding operator-() const {
        return {.Top = -Top, .Left = -Left, .Bottom = -Bottom, .Right = -Right};
    }

    [[nodiscard]] constexpr MarginPadding operator*(const glm::vec2& v) const {
        return {.Top = Top * v.y, .Left = Left * v.x, .Bottom = Bottom * v.y, .Right = Right * v.x};
    }

    template <transforms::EasingFunction E>
    [[nodiscard]] MarginPadding ValueAt(double time, const MarginPadding& startValue,
                                        const MarginPadding& endValue, double startTime,
                                        double endTime, const E& easing) const {
        return {
            .Top = utils::ValueAt(time, startValue.Top, endValue.Top, startTime, endTime, easing),
            .Left =
                utils::ValueAt(time, startValue.Left, endValue.Left, startTime, endTime, easing),
            .Bottom = utils::ValueAt(time, startValue.Bottom, endValue.Bottom, startTime, endTime,
                                     easing),
            .Right =
                utils::ValueAt(time, startValue.Right, endValue.Right, startTime, endTime, easing),
        };
    }
};

static_assert(utils::Interpolable<MarginPadding>);

} // namespace panlid::graphics
