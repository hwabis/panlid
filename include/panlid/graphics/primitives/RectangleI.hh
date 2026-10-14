#pragma once

#include "panlid/graphics/primitives/RectangleF.hh"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

namespace panlid::graphics::primitives {

struct RectangleI {
    int X{};
    int Y{};
    int Width{};
    int Height{};

    [[nodiscard]] static constexpr RectangleI Empty() { return {}; }

    [[nodiscard]] constexpr glm::ivec2 GetLocation() const { return {X, Y}; }
    [[nodiscard]] constexpr glm::ivec2 GetSize() const { return {Width, Height}; }

    [[nodiscard]] constexpr int GetLeft() const { return X; }
    [[nodiscard]] constexpr int GetTop() const { return Y; }
    [[nodiscard]] constexpr int GetRight() const { return X + Width; }
    [[nodiscard]] constexpr int GetBottom() const { return Y + Height; }

    [[nodiscard]] constexpr glm::ivec2 GetTopLeft() const { return {GetLeft(), GetTop()}; }
    [[nodiscard]] constexpr glm::ivec2 GetTopRight() const { return {GetRight(), GetTop()}; }
    [[nodiscard]] constexpr glm::ivec2 GetBottomLeft() const { return {GetLeft(), GetBottom()}; }
    [[nodiscard]] constexpr glm::ivec2 GetBottomRight() const { return {GetRight(), GetBottom()}; }

    [[nodiscard]] constexpr bool IsEmpty() const { return Width <= 0 || Height <= 0; }

    [[nodiscard]] bool operator==(const RectangleI&) const = default;

    [[nodiscard]] constexpr bool Contains(float x, float y) const {
        return X <= x && x < X + Width && Y <= y && y < Y + Height;
    }

    [[nodiscard]] constexpr bool Contains(const glm::vec2& pt) const {
        return Contains(pt.x, pt.y);
    }

    [[nodiscard]] constexpr bool Contains(int x, int y) const {
        return X <= x && x < X + Width && Y <= y && y < Y + Height;
    }

    [[nodiscard]] constexpr bool Contains(const glm::ivec2& pt) const {
        return Contains(pt.x, pt.y);
    }

    [[nodiscard]] constexpr bool Contains(const RectangleI& rect) const {
        return X <= rect.X && rect.X + rect.Width <= X + Width && Y <= rect.Y &&
               rect.Y + rect.Height <= Y + Height;
    }

    [[nodiscard]] constexpr int GetArea() const { return Width * Height; }

    [[nodiscard]] constexpr RectangleI GetWithPositiveExtent() const {
        RectangleI result = *this;

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

    [[nodiscard]] constexpr RectangleI Inflate(int amount) const {
        return Inflate(glm::ivec2{amount, amount});
    }

    [[nodiscard]] constexpr RectangleI Inflate(const glm::ivec2& amount) const {
        return Inflate(amount.x, amount.x, amount.y, amount.y);
    }

    [[nodiscard]] constexpr RectangleI Inflate(int left, int right, int top, int bottom) const {
        return {.X = X - left,
                .Y = Y - top,
                .Width = Width + left + right,
                .Height = Height + top + bottom};
    }

    [[nodiscard]] constexpr RectangleI Shrink(int amount) const {
        return Shrink(glm::ivec2{amount, amount});
    }

    [[nodiscard]] constexpr RectangleI Shrink(const glm::ivec2& amount) const {
        return Shrink(amount.x, amount.x, amount.y, amount.y);
    }

    [[nodiscard]] constexpr RectangleI Shrink(int left, int right, int top, int bottom) const {
        return Inflate(-left, -right, -top, -bottom);
    }

    [[nodiscard]] constexpr RectangleI Intersect(const RectangleI& rect) const {
        return Intersect(rect, *this);
    }

    [[nodiscard]] static constexpr RectangleI Intersect(const RectangleI& a, const RectangleI& b) {
        const int left = std::max(a.X, b.X);
        const int right = std::min(a.X + a.Width, b.X + b.Width);
        const int top = std::max(a.Y, b.Y);
        const int bottom = std::min(a.Y + a.Height, b.Y + b.Height);

        if (right >= left && bottom >= top) {
            return {.X = left, .Y = top, .Width = right - left, .Height = bottom - top};
        }

        return {};
    }

    [[nodiscard]] constexpr bool IntersectsWith(const RectangleI& rect) const {
        return rect.X < X + Width && X < rect.X + rect.Width && rect.Y < Y + Height &&
               Y < rect.Y + rect.Height;
    }

    [[nodiscard]] static constexpr RectangleI Union(const RectangleI& a, const RectangleI& b) {
        const int left = std::min(a.X, b.X);
        const int right = std::max(a.X + a.Width, b.X + b.Width);
        const int top = std::min(a.Y, b.Y);
        const int bottom = std::max(a.Y + a.Height, b.Y + b.Height);
        return {.X = left, .Y = top, .Width = right - left, .Height = bottom - top};
    }

    [[nodiscard]] constexpr RectangleI Offset(int x, int y) const {
        return {.X = X + x, .Y = Y + y, .Width = Width, .Height = Height};
    }

    [[nodiscard]] constexpr operator RectangleF() const {
        return {.X = static_cast<float>(X),
                .Y = static_cast<float>(Y),
                .Width = static_cast<float>(Width),
                .Height = static_cast<float>(Height)};
    }
};

// Smallest integer rectangle that contains this one.
inline RectangleI RectangleF::GetAABB() const {
    const auto xMin = static_cast<int>(std::floor(std::min(GetLeft(), GetRight())));
    const auto yMin = static_cast<int>(std::floor(std::min(GetTop(), GetBottom())));
    const auto xMax = static_cast<int>(std::ceil(std::max(GetLeft(), GetRight())));
    const auto yMax = static_cast<int>(std::ceil(std::max(GetTop(), GetBottom())));
    return {.X = xMin, .Y = yMin, .Width = xMax - xMin, .Height = yMax - yMin};
}

} // namespace panlid::graphics::primitives
