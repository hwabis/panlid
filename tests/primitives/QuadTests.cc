#include "panlid/graphics/primitives/Quad.hh"
#include "panlid/utils/Precision.hh"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <limits>

using namespace panlid::graphics::primitives;
using panlid::utils::kFloatEpsilon;

TEST_CASE("Quad contains", "[quad]") {
    const auto quad = GENERATE(
        Quad::FromRectangle(0, 0, 10, 10),
        // Potential precision edge cases using the precise centre point.
        Quad::FromCorners({1513.5333F, 695.51416F}, {1679.6332F, 695.51416F},
                          {1513.5333F, 861.614F}, {1679.6332F, 861.614F}),
        // Arbitrary convex quad
        Quad::FromCorners({3, 0}, {5, 1}, {0, 5}, {7, 7}),
        // Very small quad
        Quad::FromCorners({-kFloatEpsilon, -kFloatEpsilon}, {kFloatEpsilon, -kFloatEpsilon},
                          {-kFloatEpsilon, kFloatEpsilon}, {kFloatEpsilon, kFloatEpsilon}));

    CHECK_FALSE(quad.Contains(glm::vec2{std::numeric_limits<float>::lowest()}));
    CHECK_FALSE(quad.Contains(glm::vec2{std::numeric_limits<float>::max()}));

    CHECK(quad.Contains(quad.TopLeft));
    CHECK(quad.Contains(quad.TopRight));
    CHECK(quad.Contains(quad.BottomLeft));
    CHECK(quad.Contains(quad.BottomRight));
    CHECK(quad.Contains(quad.GetCentre()));

    CHECK_FALSE(quad.Contains(quad.TopLeft + glm::vec2{-1, 0}));
    CHECK_FALSE(quad.Contains(quad.TopLeft + glm::vec2{0, -1}));
    CHECK_FALSE(quad.Contains(quad.TopLeft + glm::vec2{-1, -1}));

    CHECK_FALSE(quad.Contains(quad.TopRight + glm::vec2{1, 0}));
    CHECK_FALSE(quad.Contains(quad.TopRight + glm::vec2{0, -1}));
    CHECK_FALSE(quad.Contains(quad.TopRight + glm::vec2{1, -1}));

    CHECK_FALSE(quad.Contains(quad.BottomLeft + glm::vec2{-1, 0}));
    CHECK_FALSE(quad.Contains(quad.BottomLeft + glm::vec2{0, 1}));
    CHECK_FALSE(quad.Contains(quad.BottomLeft + glm::vec2{-1, 1}));

    CHECK_FALSE(quad.Contains(quad.BottomRight + glm::vec2{1, 0}));
    CHECK_FALSE(quad.Contains(quad.BottomRight + glm::vec2{0, 1}));
    CHECK_FALSE(quad.Contains(quad.BottomRight + glm::vec2{1}));

    CHECK(quad.Contains((quad.TopLeft + quad.TopRight) / 2.0F));
    CHECK(quad.Contains((quad.BottomLeft + quad.BottomRight) / 2.0F));
    CHECK(quad.Contains((quad.TopLeft + quad.BottomLeft) / 2.0F));
    CHECK(quad.Contains((quad.TopRight + quad.BottomRight) / 2.0F));
}

TEST_CASE("Quad contains: zero height", "[quad]") {
    const auto quad = Quad::FromRectangle(-2, 0, 4, 0);

    CHECK_FALSE(quad.Contains({-5, -5}));
    CHECK_FALSE(quad.Contains({-5, 5}));
    CHECK_FALSE(quad.Contains({5, -5}));
    CHECK_FALSE(quad.Contains({5, 5}));

    CHECK(quad.Contains({-2, 0}));
    CHECK(quad.Contains({0, 0}));
    CHECK(quad.Contains({2, 0}));
}

TEST_CASE("Quad contains: zero width", "[quad]") {
    const auto quad = Quad::FromRectangle(0, -2, 0, 4);

    CHECK_FALSE(quad.Contains({-5, -5}));
    CHECK_FALSE(quad.Contains({-5, 5}));
    CHECK_FALSE(quad.Contains({5, -5}));
    CHECK_FALSE(quad.Contains({5, 5}));

    CHECK(quad.Contains({0, -2}));
    CHECK(quad.Contains({0, 0}));
    CHECK(quad.Contains({0, 2}));
}

TEST_CASE("Quad contains: zero size", "[quad]") {
    const auto quad = Quad::FromRectangle(0, 0, 0, 0);

    CHECK_FALSE(quad.Contains({-5, -5}));
    CHECK_FALSE(quad.Contains({-5, 5}));
    CHECK_FALSE(quad.Contains({5, -5}));
    CHECK_FALSE(quad.Contains({5, 5}));

    CHECK(quad.Contains({0, 0}));
}

TEST_CASE("Quad from negative-sized rectangle", "[quad]") {
    const auto quad = Quad::FromRectangle(RectangleF{-5, 5, 10, -10});

    CHECK(quad.Contains({0, 0}));

    CHECK(quad.Contains({-5, -5}));
    CHECK(quad.Contains({-5, 5}));
    CHECK(quad.Contains({5, -5}));
    CHECK(quad.Contains({5, 5}));

    CHECK_FALSE(quad.Contains({-15, -15}));
    CHECK_FALSE(quad.Contains({-15, 15}));
    CHECK_FALSE(quad.Contains({15, -15}));
    CHECK_FALSE(quad.Contains({15, 15}));
}

TEST_CASE("Flipped quad still contains its points", "[quad]") {
    auto quad = Quad::FromRectangle(-5, -5, 10, 10);
    // C# has no *= on Quad either; it is derived from operator*.
    quad = quad * glm::mat3{-1, 0, 0, 0, 1, 0, 0, 0, 1};

    CHECK(quad.Contains({0, 0}));

    CHECK(quad.Contains({-5, -5}));
    CHECK(quad.Contains({-5, 5}));
    CHECK(quad.Contains({5, -5}));
    CHECK(quad.Contains({5, 5}));

    CHECK_FALSE(quad.Contains({-15, -15}));
    CHECK_FALSE(quad.Contains({-15, 15}));
    CHECK_FALSE(quad.Contains({15, -15}));
    CHECK_FALSE(quad.Contains({15, 15}));
}

// Not in the C# tests. The flip matrix above is symmetric, so it can't catch a
// wrong matrix convention. This one can: C# Row2 is the translation, and in glm
// that is column 2.
TEST_CASE("Quad times matrix: translation lives in column 2", "[quad]") {
    glm::mat3 m{1.0F};
    m[2] = glm::vec3{10, 20, 1};

    const auto moved = Quad::FromRectangle(0, 0, 5, 5) * m;

    CHECK(moved == Quad::FromRectangle(10, 20, 5, 5));
}

TEST_CASE("Quad area", "[quad]") {
    const auto [quad, expected] = GENERATE(table<Quad, float>({
        // Point
        {Quad::FromRectangle(0, 0, 0, 0), 0.0F},

        // Lines
        {Quad::FromRectangle(0, 0, 100, 0), 0.0F},
        {Quad::FromRectangle(0, 0, 0, 100), 0.0F},
        {Quad::FromRectangle(0, 0, 100, 1), 100.0F},
        {Quad::FromRectangle(0, 0, 1, 100), 100.0F},

        // Simple quads
        {Quad::FromRectangle(0, 0, 10, 10), 100.0F},
        {Quad::FromRectangle(0, 0, 10, 5), 50.0F},
        {Quad::FromRectangle(0, 0, 5, 10), 50.0F},

        // Rotated simple quads
        {Quad::FromRectangle(10, 10, -10, -10), 100.0F},
        {Quad::FromRectangle(10, 5, -10, -5), 50.0F},
        {Quad::FromRectangle(5, 10, -5, -10), 50.0F},

        // Self-intersecting quads
        {Quad::FromCorners({0, 5}, {0, -5}, {-5, 0}, {5, 0}), 0.0F},
        {Quad::FromCorners({0, 5}, {0, -5}, {0, 0}, {5, 0}), 12.5F},
    }));

    CAPTURE(expected);
    REQUIRE_THAT(quad.GetArea(), Catch::Matchers::WithinAbs(expected, 0.1));
}
