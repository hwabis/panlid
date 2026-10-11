#pragma once

#include "panlid/graphics/MarginPadding.hh"
#include "panlid/graphics/primitives/RectangleI.hh"
#include "panlid/graphics/transforms/EasingFunction.hh"
#include "panlid/utils/Interpolable.hh"
#include <algorithm>
#include <cmath>
#include <glm/vec2.hpp>

namespace panlid::graphics::primitives {

struct RectangleF {
    float X{};
    float Y{};
    float Width{};
    float Height{};

    constexpr RectangleF() = default;
    constexpr RectangleF(float x, float y, float width, float height)
        : X(x), Y(y), Width(width), Height(height) {}
    constexpr RectangleF(glm::vec2 location, glm::vec2 size)
        : X(location.x), Y(location.y), Width(size.x), Height(size.y) {}

    [[nodiscard]] constexpr glm::vec2 GetLocation() const { return {X, Y}; }
    constexpr void SetLocation(glm::vec2 value) {
        X = value.x;
        Y = value.y;
    }

    [[nodiscard]] constexpr glm::vec2 GetSize() const { return {Width, Height}; }
    constexpr void SetSize(glm::vec2 value) {
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
    [[nodiscard]] constexpr glm::vec2 GetCentre() const {
        return {X + Width * 0.5F, Y + Height * 0.5F};
    }

    [[nodiscard]] constexpr bool IsEmpty() const { return Width <= 0.0F || Height <= 0.0F; }

    constexpr bool operator==(const RectangleF& other) const = default;

    [[nodiscard]] constexpr RectangleF operator*(float scale) const {
        return {X * scale, Y * scale, Width * scale, Height * scale};
    }
    [[nodiscard]] constexpr RectangleF operator/(float scale) const {
        return {X / scale, Y / scale, Width / scale, Height / scale};
    }
    [[nodiscard]] constexpr RectangleF operator*(glm::vec2 scale) const {
        return {X * scale.x, Y * scale.y, Width * scale.x, Height * scale.y};
    }
    [[nodiscard]] constexpr RectangleF operator/(glm::vec2 scale) const {
        return {X / scale.x, Y / scale.y, Width / scale.x, Height / scale.y};
    }

    [[nodiscard]] constexpr bool Contains(float x, float y) const {
        return X <= x && x < X + Width && Y <= y && y < Y + Height;
    }
    [[nodiscard]] constexpr bool Contains(glm::vec2 pt) const { return Contains(pt.x, pt.y); }
    [[nodiscard]] constexpr bool Contains(glm::ivec2 pt) const {
        return Contains(static_cast<float>(pt.x), static_cast<float>(pt.y));
    }
    [[nodiscard]] constexpr bool Contains(const RectangleF& rect) const {
        return X <= rect.X && rect.X + rect.Width <= X + Width && Y <= rect.Y &&
               rect.Y + rect.Height <= Y + Height;
    }

    [[nodiscard]] constexpr float GetArea() const { return Width * Height; }

    [[nodiscard]] constexpr RectangleF GetWithPositiveExtent() const {
        RectangleF result = *this;
        if (result.Width < 0.0F) {
            result.Width = -result.Width;
            result.X -= result.Width;
        }
        if (result.Height < 0.0F) {
            result.Height = -result.Height;
            result.Y -= result.Height;
        }
        return result;
    }

    [[nodiscard]] constexpr RectangleF Inflate(float amount) const {
        return Inflate(glm::vec2{amount, amount});
    }
    [[nodiscard]] constexpr RectangleF Inflate(glm::vec2 amount) const {
        return Inflate(MarginPadding{
            .Top = amount.y, .Left = amount.x, .Bottom = amount.y, .Right = amount.x});
    }
    [[nodiscard]] constexpr RectangleF Inflate(const MarginPadding& amount) const {
        return {X - amount.Left, Y - amount.Top, Width + amount.GetTotalHorizontal(),
                Height + amount.GetTotalVertical()};
    }

    [[nodiscard]] constexpr RectangleF Shrink(float amount) const {
        return Shrink(glm::vec2{amount, amount});
    }
    [[nodiscard]] constexpr RectangleF Shrink(glm::vec2 amount) const {
        return Shrink(MarginPadding{
            .Top = amount.y, .Left = amount.x, .Bottom = amount.y, .Right = amount.x});
    }
    [[nodiscard]] constexpr RectangleF Shrink(const MarginPadding& amount) const {
        return Inflate(-amount);
    }

    constexpr void Intersect(const RectangleF& rect) {
        RectangleF ef = Intersect(rect, *this);
        X = ef.X;
        Y = ef.Y;
        Width = ef.Width;
        Height = ef.Height;
    }

    [[nodiscard]] static constexpr RectangleF Intersect(const RectangleF& a, const RectangleF& b) {
        float x = std::max(a.X, b.X);
        float maxX = std::min(a.X + a.Width, b.X + b.Width);
        float y = std::max(a.Y, b.Y);
        float maxY = std::min(a.Y + a.Height, b.Y + b.Height);

        if (maxX >= x && maxY >= y) {
            return {x, y, maxX - x, maxY - y};
        }
        return {};
    }

    [[nodiscard]] constexpr bool IntersectsWith(const RectangleF& rect) const {
        return rect.X <= X + Width && X <= rect.X + rect.Width && rect.Y <= Y + Height &&
               Y <= rect.Y + rect.Height;
    }

    [[nodiscard]] constexpr bool IntersectsWith(const RectangleI& rect) const {
        return static_cast<float>(rect.X) <= X + Width &&
               X <= static_cast<float>(rect.X + rect.Width) &&
               static_cast<float>(rect.Y) <= Y + Height &&
               Y <= static_cast<float>(rect.Y + rect.Height);
    }

    [[nodiscard]] static constexpr RectangleF Union(const RectangleF& a, const RectangleF& b) {
        float x = std::min(a.X, b.X);
        float maxX = std::max(a.X + a.Width, b.X + b.Width);
        float y = std::min(a.Y, b.Y);
        float maxY = std::max(a.Y + a.Height, b.Y + b.Height);
        return {x, y, maxX - x, maxY - y};
    }

    [[nodiscard]] constexpr RectangleF Offset(glm::vec2 pos) const { return Offset(pos.x, pos.y); }
    [[nodiscard]] constexpr RectangleF Offset(float x, float y) const {
        return {X + x, Y + y, Width, Height};
    }

    [[nodiscard]] constexpr float DETAILDistanceSquared(glm::vec2 localSpacePos) const {
        glm::vec2 dist{std::max({0.0F, localSpacePos.x - GetRight(), GetLeft() - localSpacePos.x}),
                       std::max({0.0F, localSpacePos.y - GetBottom(), GetTop() - localSpacePos.y})};
        return dist.x * dist.x + dist.y * dist.y;
    }

    [[nodiscard]] float DETAILDistanceExponentiated(glm::vec2 localSpacePos, float exponent) const {
        float distX = std::max({0.0F, localSpacePos.x - GetRight(), GetLeft() - localSpacePos.x});
        float distY = std::max({0.0F, localSpacePos.y - GetBottom(), GetTop() - localSpacePos.y});
        return std::pow(distX, exponent) + std::pow(distY, exponent);
    }

    [[nodiscard]] static constexpr RectangleF FromLTRB(float left, float top, float right,
                                                       float bottom) {
        return {left, top, right - left, bottom - top};
    }

    [[nodiscard]] constexpr RectangleF RelativeIn(const RectangleF& other) const {
        float scaleX = Width / other.Width;
        float scaleY = Height / other.Height;
        return {(X - other.X) / other.Width, (Y - other.Y) / other.Height, scaleX, scaleY};
    }

    [[nodiscard]] RectangleF Normalize() const {
        return {std::min(GetLeft(), GetRight()), std::min(GetTop(), GetBottom()), std::abs(Width),
                std::abs(Height)};
    }

    constexpr RectangleF(const RectangleI& r)
        : X(static_cast<float>(r.X)), Y(static_cast<float>(r.Y)),
          Width(static_cast<float>(r.Width)), Height(static_cast<float>(r.Height)) {}

    template <graphics::transforms::EasingFunction TEasing>
    [[nodiscard]] static RectangleF ValueAt(double time, const RectangleF& val1,
                                            const RectangleF& val2, double startTime,
                                            double endTime, const TEasing& easing) {
        const auto current = static_cast<float>(time - startTime);
        const auto duration = static_cast<float>(endTime - startTime);

        if (duration == 0 || current == 0) {
            return val1;
        }

        const auto t = static_cast<float>(easing.ApplyEasing(current / duration));

        return {val1.X + t * (val2.X - val1.X), val1.Y + t * (val2.Y - val1.Y),
                val1.Width + t * (val2.Width - val1.Width),
                val1.Height + t * (val2.Height - val1.Height)};
    }
};

static_assert(utils::Interpolable<RectangleF>);

} // namespace panlid::graphics::primitives
