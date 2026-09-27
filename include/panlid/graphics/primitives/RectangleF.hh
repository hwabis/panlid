#pragma once

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

    [[nodiscard]] constexpr glm::vec2 Location() const { return {X, Y}; }
    constexpr void SetLocation(glm::vec2 value) {
        X = value.x;
        Y = value.y;
    }

    [[nodiscard]] constexpr glm::vec2 Size() const { return {Width, Height}; }
    constexpr void SetSize(glm::vec2 value) {
        Width = value.x;
        Height = value.y;
    }

    [[nodiscard]] constexpr float Left() const { return X; }
    [[nodiscard]] constexpr float Top() const { return Y; }
    [[nodiscard]] constexpr float Right() const { return X + Width; }
    [[nodiscard]] constexpr float Bottom() const { return Y + Height; }

    [[nodiscard]] constexpr glm::vec2 TopLeft() const { return {Left(), Top()}; }
    [[nodiscard]] constexpr glm::vec2 TopRight() const { return {Right(), Top()}; }
    [[nodiscard]] constexpr glm::vec2 BottomLeft() const { return {Left(), Bottom()}; }
    [[nodiscard]] constexpr glm::vec2 BottomRight() const { return {Right(), Bottom()}; }
    [[nodiscard]] constexpr glm::vec2 Centre() const {
        return {X + Width * 0.5F, Y + Height * 0.5F};
    }

    [[nodiscard]] constexpr bool IsEmpty() const { return Width <= 0.0F || Height <= 0.0F; }

    constexpr bool operator==(const RectangleF& other) const = default;

    constexpr RectangleF operator*(float scale) const {
        return {X * scale, Y * scale, Width * scale, Height * scale};
    }
    constexpr RectangleF operator/(float scale) const {
        return {X / scale, Y / scale, Width / scale, Height / scale};
    }
    constexpr RectangleF operator*(glm::vec2 scale) const {
        return {X * scale.x, Y * scale.y, Width * scale.x, Height * scale.y};
    }
    constexpr RectangleF operator/(glm::vec2 scale) const {
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

    [[nodiscard]] constexpr float Area() const { return Width * Height; }

    [[nodiscard]] constexpr RectangleF WithPositiveExtent() const {
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

    // detail
    [[nodiscard]] float DistanceSquared(glm::vec2 localSpacePos) const {
        glm::vec2 dist{std::max({0.0F, localSpacePos.x - Right(), Left() - localSpacePos.x}),
                       std::max({0.0F, localSpacePos.y - Bottom(), Top() - localSpacePos.y})};
        return dist.x * dist.x + dist.y * dist.y;
    }

    // detail
    [[nodiscard]] float DistanceExponentiated(glm::vec2 localSpacePos, float exponent) const {
        float distX = std::max({0.0F, localSpacePos.x - Right(), Left() - localSpacePos.x});
        float distY = std::max({0.0F, localSpacePos.y - Bottom(), Top() - localSpacePos.y});
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
        return {std::min(Left(), Right()), std::min(Top(), Bottom()), std::abs(Width),
                std::abs(Height)};
    }
};

} // namespace panlid::graphics::primitives
