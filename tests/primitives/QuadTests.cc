#include "panlid/extensions/MatrixExtensions.hh"
#include "panlid/graphics/primitives/Quad.hh"
#include "panlid/graphics/primitives/RectangleF.hh"
#include "panlid/utils/Precision.hh"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <glm/glm.hpp>
#include <limits>

using panlid::graphics::primitives::Quad;
using panlid::graphics::primitives::RectangleF;
using panlid::utils::kFloatEpsilon;

namespace {

Quad Shear(const Quad& quad, const glm::vec2& amount) {
    glm::mat3 matrix{1.F};
    panlid::extensions::ShearFromLeft(matrix, amount / quad.GetSize());
    return quad * matrix;
}

} // namespace

TEST_CASE("Contains", "[Quad]") {
    const auto quad = GENERATE(
        Quad::FromRectangle(0, 0, 10, 10),
        // Potential precision edge cases using the precise centre point.
        Quad::FromCorners({1513.5333F, 695.51416F}, {1679.6332F, 695.51416F},
                          {1513.5333F, 861.614F}, {1679.6332F, 861.614F}),
        // Arbitrary convex quad.
        Quad::FromCorners({3.F, 0.F}, {5.F, 1.F}, {0.F, 5.F}, {7.F, 7.F}),
        // Very small quad.
        Quad::FromCorners({-kFloatEpsilon, -kFloatEpsilon}, {kFloatEpsilon, -kFloatEpsilon},
                          {-kFloatEpsilon, kFloatEpsilon}, {kFloatEpsilon, kFloatEpsilon}));

    REQUIRE_FALSE(quad.Contains(glm::vec2{std::numeric_limits<float>::lowest()}));
    REQUIRE_FALSE(quad.Contains(glm::vec2{std::numeric_limits<float>::max()}));

    REQUIRE(quad.Contains(quad.TopLeft));
    REQUIRE(quad.Contains(quad.TopRight));
    REQUIRE(quad.Contains(quad.BottomLeft));
    REQUIRE(quad.Contains(quad.BottomRight));
    REQUIRE(quad.Contains(quad.GetCentre()));

    REQUIRE_FALSE(quad.Contains(quad.TopLeft + glm::vec2{-1.F, 0.F}));
    REQUIRE_FALSE(quad.Contains(quad.TopLeft + glm::vec2{0.F, -1.F}));
    REQUIRE_FALSE(quad.Contains(quad.TopLeft + glm::vec2{-1.F, -1.F}));

    REQUIRE_FALSE(quad.Contains(quad.TopRight + glm::vec2{1.F, 0.F}));
    REQUIRE_FALSE(quad.Contains(quad.TopRight + glm::vec2{0.F, -1.F}));
    REQUIRE_FALSE(quad.Contains(quad.TopRight + glm::vec2{1.F, -1.F}));

    REQUIRE_FALSE(quad.Contains(quad.BottomLeft + glm::vec2{-1.F, 0.F}));
    REQUIRE_FALSE(quad.Contains(quad.BottomLeft + glm::vec2{0.F, 1.F}));
    REQUIRE_FALSE(quad.Contains(quad.BottomLeft + glm::vec2{-1.F, 1.F}));

    REQUIRE_FALSE(quad.Contains(quad.BottomRight + glm::vec2{1.F, 0.F}));
    REQUIRE_FALSE(quad.Contains(quad.BottomRight + glm::vec2{0.F, 1.F}));
    REQUIRE_FALSE(quad.Contains(quad.BottomRight + glm::vec2{1.F}));

    REQUIRE(quad.Contains((quad.TopLeft + quad.TopRight) / 2.F));
    REQUIRE(quad.Contains((quad.BottomLeft + quad.BottomRight) / 2.F));
    REQUIRE(quad.Contains((quad.TopLeft + quad.BottomLeft) / 2.F));
    REQUIRE(quad.Contains((quad.TopRight + quad.BottomRight) / 2.F));
}

TEST_CASE("Contains_ZeroHeightQuad", "[Quad]") {
    const auto quad = Quad::FromRectangle(-2, 0, 4, 0);

    REQUIRE_FALSE(quad.Contains({-5.F, -5.F}));
    REQUIRE_FALSE(quad.Contains({-5.F, 5.F}));
    REQUIRE_FALSE(quad.Contains({5.F, -5.F}));
    REQUIRE_FALSE(quad.Contains({5.F, 5.F}));

    REQUIRE(quad.Contains({-2.F, 0.F}));
    REQUIRE(quad.Contains({0.F, 0.F}));
    REQUIRE(quad.Contains({2.F, 0.F}));
}

TEST_CASE("Contains_ZeroWidthQuad", "[Quad]") {
    const auto quad = Quad::FromRectangle(0, -2, 0, 4);

    REQUIRE_FALSE(quad.Contains({-5.F, -5.F}));
    REQUIRE_FALSE(quad.Contains({-5.F, 5.F}));
    REQUIRE_FALSE(quad.Contains({5.F, -5.F}));
    REQUIRE_FALSE(quad.Contains({5.F, 5.F}));

    REQUIRE(quad.Contains({0.F, -2.F}));
    REQUIRE(quad.Contains({0.F, 0.F}));
    REQUIRE(quad.Contains({0.F, 2.F}));
}

TEST_CASE("Contains_ZeroSizedQuad", "[Quad]") {
    const auto quad = Quad::FromRectangle(0, 0, 0, 0);

    REQUIRE_FALSE(quad.Contains({-5.F, -5.F}));
    REQUIRE_FALSE(quad.Contains({-5.F, 5.F}));
    REQUIRE_FALSE(quad.Contains({5.F, -5.F}));
    REQUIRE_FALSE(quad.Contains({5.F, 5.F}));

    REQUIRE(quad.Contains({0.F, 0.F}));
}

TEST_CASE("FromNegativeSizedRectangle", "[Quad]") {
    const RectangleF rectangle{.X = -5, .Y = 5, .Width = 10, .Height = -10};
    const auto quad = Quad::FromRectangle(rectangle);

    REQUIRE(quad.Contains({0.F, 0.F}));

    REQUIRE(quad.Contains({-5.F, -5.F}));
    REQUIRE(quad.Contains({-5.F, 5.F}));
    REQUIRE(quad.Contains({5.F, -5.F}));
    REQUIRE(quad.Contains({5.F, 5.F}));

    REQUIRE_FALSE(quad.Contains({-15.F, -15.F}));
    REQUIRE_FALSE(quad.Contains({-15.F, 15.F}));
    REQUIRE_FALSE(quad.Contains({15.F, -15.F}));
    REQUIRE_FALSE(quad.Contains({15.F, 15.F}));
}

TEST_CASE("FlippedQuad", "[Quad]") {
    auto quad = Quad::FromRectangle(-5, -5, 10, 10);
    quad = quad * glm::mat3{-1.F, 0.F, 0.F, 0.F, 1.F, 0.F, 0.F, 0.F, 1.F};

    REQUIRE(quad.Contains({0.F, 0.F}));

    REQUIRE(quad.Contains({-5.F, -5.F}));
    REQUIRE(quad.Contains({-5.F, 5.F}));
    REQUIRE(quad.Contains({5.F, -5.F}));
    REQUIRE(quad.Contains({5.F, 5.F}));

    REQUIRE_FALSE(quad.Contains({-15.F, -15.F}));
    REQUIRE_FALSE(quad.Contains({-15.F, 15.F}));
    REQUIRE_FALSE(quad.Contains({15.F, -15.F}));
    REQUIRE_FALSE(quad.Contains({15.F, 15.F}));
}

TEST_CASE("Area", "[Quad]") {

    const auto [quad, expected] = GENERATE(table<Quad, float>({
        // Point
        {Quad::FromRectangle(0, 0, 0, 0), 0.F},

        // Lines
        {Quad::FromRectangle(0, 0, 100, 0), 0.F},
        {Quad::FromRectangle(0, 0, 0, 100), 0.F},
        {Quad::FromRectangle(0, 0, 100, 1), 100.F},
        {Quad::FromRectangle(0, 0, 1, 100), 100.F},

        // Simple quads
        {Quad::FromRectangle(0, 0, 10, 10), 100.F},
        {Quad::FromRectangle(0, 0, 10, 5), 50.F},
        {Quad::FromRectangle(0, 0, 5, 10), 50.F},

        // Rotated simple quads
        {Quad::FromRectangle(10, 10, -10, -10), 100.F},
        {Quad::FromRectangle(10, 5, -10, -5), 50.F},
        {Quad::FromRectangle(5, 10, -5, -10), 50.F},

        // Sheared quads
        {Shear(Quad::FromRectangle(0, 0, 10, 10), glm::vec2{2.F}), 100.F},
        {Shear(Quad::FromRectangle(0, 0, 10, 10), glm::vec2{-2.F}), 100.F},

        // Sheared rotated quads
        {Shear(Quad::FromRectangle(10, 10, -10, -10), glm::vec2{2.F}), 100.F},
        {Shear(Quad::FromRectangle(10, 5, -10, -5), glm::vec2{2.F}), 50.F},
        {Shear(Quad::FromRectangle(5, 10, -5, -10), glm::vec2{2.F}), 50.F},

        // Self-intersecting quads
        {Quad::FromCorners({0.F, 5.F}, {0.F, -5.F}, {-5.F, 0.F}, {5.F, 0.F}), 0.F},
        {Quad::FromCorners({0.F, 5.F}, {0.F, -5.F}, {0.F, 0.F}, {5.F, 0.F}), 12.5F},
    }));

    REQUIRE_THAT(quad.GetArea(), Catch::Matchers::WithinAbs(expected, 0.1));
}
