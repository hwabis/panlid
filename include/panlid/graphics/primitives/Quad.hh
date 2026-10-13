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
    glm::vec2 TopLeft{};
    glm::vec2 BottomLeft{};
    glm::vec2 BottomRight{};
    glm::vec2 TopRight{};

    [[nodiscard]] static constexpr Quad FromCorners(glm::vec2 topLeft, glm::vec2 topRight,
                                                    glm::vec2 bottomLeft, glm::vec2 bottomRight) {
        return {.TopLeft = topLeft,
                .BottomLeft = bottomLeft,
                .BottomRight = bottomRight,
                .TopRight = topRight};
    }

    [[nodiscard]] static constexpr Quad FromRectangle(float x, float y, float width, float height) {
        return FromCorners({x, y}, {x + width, y}, {x, y + height}, {x + width, y + height});
    }

    [[nodiscard]] static Quad FromRectangle(const RectangleF& r) {
        return FromCorners({r.GetLeft(), r.GetTop()}, {r.GetRight(), r.GetTop()},
                           {r.GetLeft(), r.GetBottom()}, {r.GetRight(), r.GetBottom()});
    }

    [[nodiscard]] Quad operator*(const glm::mat3& m) const {
        return FromCorners(transform(TopLeft, m), transform(TopRight, m), transform(BottomLeft, m),
                           transform(BottomRight, m));
    }

    [[nodiscard]] glm::mat2 GetBasisTransform() const {
        glm::vec2 row0 = TopRight - TopLeft;
        glm::vec2 row1 = BottomLeft - TopLeft;

        if (row0 != glm::vec2{0.0F}) {
            row0 /= glm::dot(row0, row0);
        }
        if (row1 != glm::vec2{0.0F}) {
            row1 /= glm::dot(row1, row1);
        }

        return glm::mat2{row0.x, row0.y, row1.x, row1.y};
    }

    [[nodiscard]] glm::vec2 GetCentre() const {
        return (TopLeft + TopRight + BottomLeft + BottomRight) / 4.0F;
    }
    [[nodiscard]] glm::vec2 GetSize() const { return {GetWidth(), GetHeight()}; }

    [[nodiscard]] float GetWidth() const { return glm::distance(TopLeft, TopRight); }
    [[nodiscard]] float GetHeight() const { return glm::distance(TopLeft, BottomLeft); }

    [[nodiscard]] std::array<glm::vec2, 4> GetVertices() const {
        return {TopLeft, BottomLeft, BottomRight, TopRight};
    }

    [[nodiscard]] RectangleI GetAABB() const {
        const int xMin = static_cast<int>(
            std::floor(std::min({TopLeft.x, TopRight.x, BottomLeft.x, BottomRight.x})));
        const int yMin = static_cast<int>(
            std::floor(std::min({TopLeft.y, TopRight.y, BottomLeft.y, BottomRight.y})));
        const int xMax = static_cast<int>(
            std::ceil(std::max({TopLeft.x, TopRight.x, BottomLeft.x, BottomRight.x})));
        const int yMax = static_cast<int>(
            std::ceil(std::max({TopLeft.y, TopRight.y, BottomLeft.y, BottomRight.y})));
        return RectangleI{xMin, yMin, xMax - xMin, yMax - yMin};
    }

    [[nodiscard]] RectangleF GetAABBFloat() const {
        const float xMin = std::min({TopLeft.x, TopRight.x, BottomLeft.x, BottomRight.x});
        const float yMin = std::min({TopLeft.y, TopRight.y, BottomLeft.y, BottomRight.y});
        const float xMax = std::max({TopLeft.x, TopRight.x, BottomLeft.x, BottomRight.x});
        const float yMax = std::max({TopLeft.y, TopRight.y, BottomLeft.y, BottomRight.y});
        return RectangleF{xMin, yMin, xMax - xMin, yMax - yMin};
    }

    [[nodiscard]] bool Contains(glm::vec2 pos) const {
        if (GetWidth() == 0 && GetHeight() == 0) {
            return pos == TopLeft;
        }

        const float perpDot1 = perpDot(BottomLeft - TopLeft, pos - TopLeft);
        if (std::isnan(perpDot1)) {
            return false;
        }

        const float perpDot2 = perpDot(BottomRight - BottomLeft, pos - BottomLeft);
        if (std::isnan(perpDot2) || perpDot1 * perpDot2 < 0) {
            return false;
        }

        const float perpDot3 = perpDot(TopRight - BottomRight, pos - BottomRight);
        if (std::isnan(perpDot3) || perpDot1 * perpDot3 < 0 || perpDot2 * perpDot3 < 0) {
            return false;
        }

        const float perpDot4 = perpDot(TopLeft - TopRight, pos - TopRight);
        // NOLINTBEGIN(readability-simplify-boolean-expr)
        if (std::isnan(perpDot4) || perpDot1 * perpDot4 < 0 || perpDot2 * perpDot4 < 0 ||
            perpDot3 * perpDot4 < 0) {
            return false;
        }

        return true;
        // NOLINTEND(readability-simplify-boolean-expr)
    }

    [[nodiscard]] float GetArea() const {
        const auto v = GetVertices();
        float sum = 0;
        for (std::size_t i = 0; i < v.size(); ++i) {
            const glm::vec2 a = v[i];
            const glm::vec2 b = v[(i + 1) % v.size()];
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
    [[nodiscard]] static float perpDot(glm::vec2 a, glm::vec2 b) { return a.x * b.y - a.y * b.x; }
    [[nodiscard]] static glm::vec2 transform(glm::vec2 v, const glm::mat3& m) {
        const glm::vec3 r = m * glm::vec3{v, 1.0F};
        return {r.x, r.y};
    }
};

static_assert(sizeof(Quad) == 4 * sizeof(glm::vec2));

} // namespace panlid::graphics::primitives
