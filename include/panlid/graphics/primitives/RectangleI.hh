#pragma once

#include <algorithm>
#include <glm/vec2.hpp>

namespace panlid::graphics::primitives {

struct RectangleI {
    int X{};
    int Y{};
    int Width{};
    int Height{};

    constexpr RectangleI() = default;
    constexpr RectangleI(int x, int y, int width, int height)
        : X(x), Y(y), Width(width), Height(height) {}

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

    constexpr bool operator==(const RectangleI& other) const = default;

    [[nodiscard]] constexpr bool Contains(float x, float y) const {
        return static_cast<float>(X) <= x && x < static_cast<float>(X + Width) &&
               static_cast<float>(Y) <= y && y < static_cast<float>(Y + Height);
    }
    [[nodiscard]] constexpr bool Contains(glm::vec2 pt) const { return Contains(pt.x, pt.y); }
    [[nodiscard]] constexpr bool Contains(int x, int y) const {
        return X <= x && x < X + Width && Y <= y && y < Y + Height;
    }
    [[nodiscard]] constexpr bool Contains(glm::ivec2 pt) const { return Contains(pt.x, pt.y); }
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
    [[nodiscard]] constexpr RectangleI Inflate(glm::ivec2 amount) const {
        return Inflate(amount.x, amount.x, amount.y, amount.y);
    }
    [[nodiscard]] constexpr RectangleI Inflate(int left, int right, int top, int bottom) const {
        return {X - left, Y - top, Width + left + right, Height + top + bottom};
    }

    [[nodiscard]] constexpr RectangleI Shrink(int amount) const {
        return Shrink(glm::ivec2{amount, amount});
    }
    [[nodiscard]] constexpr RectangleI Shrink(glm::ivec2 amount) const {
        return Shrink(amount.x, amount.x, amount.y, amount.y);
    }
    [[nodiscard]] constexpr RectangleI Shrink(int left, int right, int top, int bottom) const {
        return Inflate(-left, -right, -top, -bottom);
    }

    [[nodiscard]] constexpr RectangleI Intersect(const RectangleI& rect) const {
        return Intersect(rect, *this);
    }

    [[nodiscard]] static constexpr RectangleI Intersect(const RectangleI& a, const RectangleI& b) {
        const int x = std::max(a.X, b.X);
        const int maxX = std::min(a.X + a.Width, b.X + b.Width);
        const int y = std::max(a.Y, b.Y);
        const int maxY = std::min(a.Y + a.Height, b.Y + b.Height);

        if (maxX >= x && maxY >= y) {
            return {x, y, maxX - x, maxY - y};
        }
        return {};
    }

    [[nodiscard]] constexpr bool IntersectsWith(const RectangleI& rect) const {
        return rect.X < X + Width && X < rect.X + rect.Width && rect.Y < Y + Height &&
               Y < rect.Y + rect.Height;
    }

    [[nodiscard]] static constexpr RectangleI Union(const RectangleI& a, const RectangleI& b) {
        const int x = std::min(a.X, b.X);
        const int maxX = std::max(a.X + a.Width, b.X + b.Width);
        const int y = std::min(a.Y, b.Y);
        const int maxY = std::max(a.Y + a.Height, b.Y + b.Height);
        return {x, y, maxX - x, maxY - y};
    }

    [[nodiscard]] constexpr RectangleI Offset(int x, int y) const {
        return {X + x, Y + y, Width, Height};
    }
};

} // namespace panlid::graphics::primitives
