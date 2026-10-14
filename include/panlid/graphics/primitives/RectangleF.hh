#pragma once

#include "panlid/graphics/MarginPadding.hh"
#include "panlid/graphics/transforms/EasingFunction.hh"
#include "panlid/utils/Interpolable.hh"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

namespace panlid::graphics::primitives {

struct RectangleI;

struct RectangleF {
    float X{};
    float Y{};
    float Width{};
    float Height{};

    [[nodiscard]] static constexpr RectangleF Empty() { return {}; }

    [[nodiscard]] static constexpr RectangleF FromLocationSize(const glm::vec2& location,
                                                               const glm::vec2& size) {
        return {.X = location.x, .Y = location.y, .Width = size.x, .Height = size.y};
    }

    [[nodiscard]] static constexpr RectangleF FromLTRB(float left, float top, float right,
                                                       float bottom) {
        return {.X = left, .Y = top, .Width = right - left, .Height = bottom - top};
    }

    [[nodiscard]] constexpr glm::vec2 GetLocation() const { return {X, Y}; }

    constexpr void SetLocation(const glm::vec2& value) {
        X = value.x;
        Y = value.y;
    }

    [[nodiscard]] constexpr glm::vec2 GetSize() const { return {Width, Height}; }

    constexpr void SetSize(const glm::vec2& value) {
        Width = value.x;
        Height = value.y;
    }

    [[nodiscard]] constexpr float GetLeft() const { return X; }
    [[nodiscard]] constexpr float GetTop() const { return Y; }
    [[nodiscard]] constexpr float GetRight() const { return X + Width; }
    [[nodiscard]] constexpr float GetBottom() const { return Y + Height; }

    [[nodiscard]] constexpr glm::vec2 GetTopLeft() const { return {GetLeft(), GetTop()}; }
    [[nodiscard]] constexpr glm::vec2 GetTopRight() const { return {GetRight(), GetTop()}; }
    [[nodiscard]] constexpr glm::vec2 GetBottomLeft() const { return {GetLeft(), GetBottom()}; }
    [[nodiscard]] constexpr glm::vec2 GetBottomRight() const { return {GetRight(), GetBottom()}; }
    [[nodiscard]] constexpr glm::vec2 GetCentre() const { return {X + Width / 2, Y + Height / 2}; }

    [[nodiscard]] constexpr bool IsEmpty() const { return Width <= 0 || Height <= 0; }

    [[nodiscard]] bool operator==(const RectangleF&) const = default;

    [[nodiscard]] constexpr RectangleF operator*(float scale) const {
        return {.X = X * scale, .Y = Y * scale, .Width = Width * scale, .Height = Height * scale};
    }

    [[nodiscard]] constexpr RectangleF operator/(float scale) const {
        return {.X = X / scale, .Y = Y / scale, .Width = Width / scale, .Height = Height / scale};
    }

    [[nodiscard]] constexpr RectangleF operator*(const glm::vec2& scale) const {
        return {.X = X * scale.x,
                .Y = Y * scale.y,
                .Width = Width * scale.x,
                .Height = Height * scale.y};
    }

    [[nodiscard]] constexpr RectangleF operator/(const glm::vec2& scale) const {
        return {.X = X / scale.x,
                .Y = Y / scale.y,
                .Width = Width / scale.x,
                .Height = Height / scale.y};
    }

    [[nodiscard]] constexpr bool Contains(float x, float y) const {
        return X <= x && x < X + Width && Y <= y && y < Y + Height;
    }

    [[nodiscard]] constexpr bool Contains(const glm::vec2& pt) const {
        return Contains(pt.x, pt.y);
    }

    [[nodiscard]] constexpr bool Contains(const glm::ivec2& pt) const {
        return Contains(static_cast<float>(pt.x), static_cast<float>(pt.y));
    }

    [[nodiscard]] constexpr bool Contains(const RectangleF& rect) const {
        return X <= rect.X && rect.X + rect.Width <= X + Width && Y <= rect.Y &&
               rect.Y + rect.Height <= Y + Height;
    }

    [[nodiscard]] constexpr float GetArea() const { return Width * Height; }

    [[nodiscard]] constexpr RectangleF GetWithPositiveExtent() const {
        RectangleF result = *this;

        if (result.Width < 0) {
            result.Width = -result.Width;
            result.X -= result.Width;
        }

        if (result.Height < 0) {
            result.Height = -result.Height;
            result.Y -= result.Height;
        }

        return result;
    }

    // The centre stays in place.
    [[nodiscard]] constexpr RectangleF Inflate(float amount) const {
        return Inflate(glm::vec2{amount, amount});
    }

    [[nodiscard]] constexpr RectangleF Inflate(const glm::vec2& amount) const {
        return Inflate(MarginPadding{
            .Top = amount.y, .Left = amount.x, .Bottom = amount.y, .Right = amount.x});
    }

    [[nodiscard]] constexpr RectangleF Inflate(const MarginPadding& amount) const {
        return {.X = X - amount.Left,
                .Y = Y - amount.Top,
                .Width = Width + amount.GetTotalHorizontal(),
                .Height = Height + amount.GetTotalVertical()};
    }

    [[nodiscard]] constexpr RectangleF Shrink(float amount) const {
        return Shrink(glm::vec2{amount, amount});
    }

    [[nodiscard]] constexpr RectangleF Shrink(const glm::vec2& amount) const {
        return Shrink(MarginPadding{
            .Top = amount.y, .Left = amount.x, .Bottom = amount.y, .Right = amount.x});
    }

    [[nodiscard]] constexpr RectangleF Shrink(const MarginPadding& amount) const {
        return Inflate(-amount);
    }

    constexpr void Intersect(const RectangleF& rect) { *this = Intersect(rect, *this); }

    [[nodiscard]] static constexpr RectangleF Intersect(const RectangleF& a, const RectangleF& b) {
        const float left = std::max(a.X, b.X);
        const float right = std::min(a.X + a.Width, b.X + b.Width);
        const float top = std::max(a.Y, b.Y);
        const float bottom = std::min(a.Y + a.Height, b.Y + b.Height);

        if (right >= left && bottom >= top) {
            return {.X = left, .Y = top, .Width = right - left, .Height = bottom - top};
        }

        return {};
    }

    [[nodiscard]] constexpr bool IntersectsWith(const RectangleF& rect) const {
        return rect.X <= X + Width && X <= rect.X + rect.Width && rect.Y <= Y + Height &&
               Y <= rect.Y + rect.Height;
    }

    [[nodiscard]] static constexpr RectangleF Union(const RectangleF& a, const RectangleF& b) {
        const float left = std::min(a.X, b.X);
        const float right = std::max(a.X + a.Width, b.X + b.Width);
        const float top = std::min(a.Y, b.Y);
        const float bottom = std::max(a.Y + a.Height, b.Y + b.Height);
        return {.X = left, .Y = top, .Width = right - left, .Height = bottom - top};
    }

    [[nodiscard]] constexpr RectangleF Offset(const glm::vec2& pos) const {
        return Offset(pos.x, pos.y);
    }

    [[nodiscard]] constexpr RectangleF Offset(float x, float y) const {
        return {.X = X + x, .Y = Y + y, .Width = Width, .Height = Height};
    }

    [[nodiscard]] float DistanceSquared(const glm::vec2& localSpacePos) const {
        const glm::vec2 dist{
            std::max({0.F, localSpacePos.x - GetRight(), GetLeft() - localSpacePos.x}),
            std::max({0.F, localSpacePos.y - GetBottom(), GetTop() - localSpacePos.y}),
        };

        return glm::dot(dist, dist);
    }

    [[nodiscard]] float DistanceExponentiated(const glm::vec2& localSpacePos,
                                              float exponent) const {
        const float distX =
            std::max({0.F, localSpacePos.x - GetRight(), GetLeft() - localSpacePos.x});
        const float distY =
            std::max({0.F, localSpacePos.y - GetBottom(), GetTop() - localSpacePos.y});

        return std::pow(distX, exponent) + std::pow(distY, exponent);
    }

    // Defined in RectangleI.hh, which this header can't include.
    [[nodiscard]] RectangleI GetAABB() const;

    // The rectangle's coordinates relative to `other` (its origin and size become 0..1).
    [[nodiscard]] constexpr RectangleF RelativeIn(const RectangleF& other) const {
        return {.X = (X - other.X) / other.Width,
                .Y = (Y - other.Y) / other.Height,
                .Width = Width / other.Width,
                .Height = Height / other.Height};
    }

    [[nodiscard]] RectangleF Normalize() const {
        return {.X = std::min(GetLeft(), GetRight()),
                .Y = std::min(GetTop(), GetBottom()),
                .Width = std::abs(Width),
                .Height = std::abs(Height)};
    }

    template <transforms::EasingFunction E>
    [[nodiscard]] RectangleF ValueAt(double time, const RectangleF& start, const RectangleF& end,
                                     double startTime, double endTime, const E& easing) const {
        const auto current = static_cast<float>(time - startTime);
        const auto duration = static_cast<float>(endTime - startTime);

        if (duration == 0 || current == 0) {
            return start;
        }

        const auto t = static_cast<float>(easing.ApplyEasing(current / duration));

        return {.X = start.X + t * (end.X - start.X),
                .Y = start.Y + t * (end.Y - start.Y),
                .Width = start.Width + t * (end.Width - start.Width),
                .Height = start.Height + t * (end.Height - start.Height)};
    }
};

static_assert(utils::Interpolable<RectangleF>);

} // namespace panlid::graphics::primitives
