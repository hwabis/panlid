#pragma once

#include "panlid/graphics/primitives/RectangleF.hh"
#include "panlid/graphics/primitives/RectangleI.hh"
#include "panlid/utils/Precision.hh"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <glm/glm.hpp>

namespace panlid::graphics::primitives {

struct Quad {
    // Screen-space counter-clockwise; GetVertices() depends on this order.
    glm::vec2 TopLeft{};
    glm::vec2 BottomLeft{};
    glm::vec2 BottomRight{};
    glm::vec2 TopRight{};

    [[nodiscard]] static constexpr Quad FromCorners(const glm::vec2& topLeft,
                                                    const glm::vec2& topRight,
                                                    const glm::vec2& bottomLeft,
                                                    const glm::vec2& bottomRight) {
        return {.TopLeft = topLeft,
                .BottomLeft = bottomLeft,
                .BottomRight = bottomRight,
                .TopRight = topRight};
    }

    [[nodiscard]] static constexpr Quad FromRectangle(float x, float y, float width, float height) {
        return FromRectangle(RectangleF{.X = x, .Y = y, .Width = width, .Height = height});
    }

    [[nodiscard]] static constexpr Quad FromRectangle(const RectangleF& rectangle) {
        return FromCorners(rectangle.GetTopLeft(), rectangle.GetTopRight(),
                           rectangle.GetBottomLeft(), rectangle.GetBottomRight());
    }

    [[nodiscard]] Quad operator*(const glm::mat3& m) const {
        return FromCorners(transform(TopLeft, m), transform(TopRight, m), transform(BottomLeft, m),
                           transform(BottomRight, m));
    }

    [[nodiscard]] glm::mat2 GetBasisTransform() const {
        glm::vec2 row0 = TopRight - TopLeft;
        glm::vec2 row1 = BottomLeft - TopLeft;

        if (row0 != glm::vec2{0.F}) {
            row0 /= glm::dot(row0, row0);
        }

        if (row1 != glm::vec2{0.F}) {
            row1 /= glm::dot(row1, row1);
        }

        return glm::mat2{row0.x, row0.y, row1.x, row1.y};
    }

    [[nodiscard]] glm::vec2 GetCentre() const {
        return (TopLeft + TopRight + BottomLeft + BottomRight) / 4.F;
    }

    [[nodiscard]] glm::vec2 GetSize() const { return {GetWidth(), GetHeight()}; }

    [[nodiscard]] float GetWidth() const { return glm::distance(TopLeft, TopRight); }

    [[nodiscard]] float GetHeight() const { return glm::distance(TopLeft, BottomLeft); }

    [[nodiscard]] RectangleI GetAABB() const {
        const auto xMin = static_cast<int>(
            std::floor(std::min({TopLeft.x, TopRight.x, BottomLeft.x, BottomRight.x})));
        const auto yMin = static_cast<int>(
            std::floor(std::min({TopLeft.y, TopRight.y, BottomLeft.y, BottomRight.y})));
        const auto xMax = static_cast<int>(
            std::ceil(std::max({TopLeft.x, TopRight.x, BottomLeft.x, BottomRight.x})));
        const auto yMax = static_cast<int>(
            std::ceil(std::max({TopLeft.y, TopRight.y, BottomLeft.y, BottomRight.y})));

        return {.X = xMin, .Y = yMin, .Width = xMax - xMin, .Height = yMax - yMin};
    }

    [[nodiscard]] RectangleF GetAABBFloat() const {
        const float xMin = std::min({TopLeft.x, TopRight.x, BottomLeft.x, BottomRight.x});
        const float yMin = std::min({TopLeft.y, TopRight.y, BottomLeft.y, BottomRight.y});
        const float xMax = std::max({TopLeft.x, TopRight.x, BottomLeft.x, BottomRight.x});
        const float yMax = std::max({TopLeft.y, TopRight.y, BottomLeft.y, BottomRight.y});

        return {.X = xMin, .Y = yMin, .Width = xMax - xMin, .Height = yMax - yMin};
    }

    [[nodiscard]] std::array<glm::vec2, 4> GetVertices() const {
        return {TopLeft, BottomLeft, BottomRight, TopRight};
    }

    // Assumes a convex quad; that isn't checked.
    [[nodiscard]] bool Contains(const glm::vec2& pos) const {
        if (GetWidth() == 0 && GetHeight() == 0) {
            return pos == TopLeft;
        }

        // Walk the four edges and compare the signs of the perp dots: the point is inside iff
        // they all match (the winding may be either, e.g. after a negative scale). Zero means
        // "on the edge's line" and never rejects. Signs are compared by multiplying, so overflow
        // doesn't matter. NaN comes from Infinity - Infinity and is rejected outright.
        const std::array<float, 4> dots{
            perpDot(BottomLeft - TopLeft, pos - TopLeft),
            perpDot(BottomRight - BottomLeft, pos - BottomLeft),
            perpDot(TopRight - BottomRight, pos - BottomRight),
            perpDot(TopLeft - TopRight, pos - TopRight),
        };

        for (std::size_t i = 0; i < dots.size(); ++i) {
            if (std::isnan(dots[i])) {
                return false;
            }

            for (std::size_t j = i + 1; j < dots.size(); ++j) {
                if (dots[i] * dots[j] < 0) {
                    return false;
                }
            }
        }

        return true;
    }

    // For a self-intersecting quad this is the sum of signed areas, not the visible area.
    [[nodiscard]] float GetArea() const {
        const auto vertices = GetVertices();
        float sum = 0;

        for (std::size_t i = 0; i < vertices.size(); ++i) {
            const glm::vec2& a = vertices[i];
            const glm::vec2& b = vertices[(i + 1) % vertices.size()];
            sum += a.x * b.y - b.x * a.y;
        }

        return 0.5F * std::abs(sum);
    }

    [[nodiscard]] bool operator==(const Quad&) const = default;

    [[nodiscard]] bool AlmostEquals(const Quad& other) const {
        return utils::AlmostEquals(TopLeft, other.TopLeft) &&
               utils::AlmostEquals(TopRight, other.TopRight) &&
               utils::AlmostEquals(BottomLeft, other.BottomLeft) &&
               utils::AlmostEquals(BottomRight, other.BottomRight);
    }

  private:
    [[nodiscard]] static constexpr float perpDot(const glm::vec2& a, const glm::vec2& b) {
        return a.x * b.y - a.y * b.x;
    }

    // C# rows are glm columns, so m * v here equals v * M there.
    [[nodiscard]] static glm::vec2 transform(const glm::vec2& v, const glm::mat3& m) {
        return glm::vec2{m * glm::vec3{v, 1.F}};
    }
};

static_assert(sizeof(Quad) == 4 * sizeof(glm::vec2));

} // namespace panlid::graphics::primitives
