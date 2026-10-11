#include "panlid/graphics/Easing.hh"
#include "panlid/graphics/MarginPadding.hh"
#include "panlid/graphics/transforms/EasingFunction.hh"
#include "panlid/utils/Interpolation.hh"
#include "panlid/utils/Precision.hh"
#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <glm/vec2.hpp>
#include <limits>
#include <magic_enum/magic_enum.hpp>
#include <stdexcept>
#include <vector>

using Catch::Matchers::WithinAbs;

namespace {

struct TestClassWithValueAt {
    int I{};
    constexpr explicit TestClassWithValueAt(int i) : I(i) {}
    constexpr bool operator==(const TestClassWithValueAt& other) const = default;

    template <panlid::graphics::transforms::EasingFunction TEasing>
    [[nodiscard]] static TestClassWithValueAt
    ValueAt(double time, const TestClassWithValueAt& startValue,
            const TestClassWithValueAt& endValue, double startTime, double endTime,
            const TEasing& easing) {
        return TestClassWithValueAt(
            panlid::utils::ValueAt(time, startValue.I, endValue.I, startTime, endTime, easing));
    }
};

} // namespace

TEST_CASE("EasingStartsAtZero", "[Interpolation]") {
    for (auto easing : magic_enum::enum_values<panlid::graphics::Easing>()) {
        CAPTURE(static_cast<int>(easing));
        CHECK_THAT(panlid::utils::ApplyEasing(easing, 0),
                   WithinAbs(0, panlid::utils::kDoubleEpsilon));
    }
}

TEST_CASE("EasingEndsAtOne", "[Interpolation]") {
    for (auto easing : magic_enum::enum_values<panlid::graphics::Easing>()) {
        CAPTURE(static_cast<int>(easing));
        CHECK_THAT(panlid::utils::ApplyEasing(easing, 1),
                   WithinAbs(1, panlid::utils::kDoubleEpsilon));
    }
}

TEST_CASE("Lerp", "[Interpolation]") {
    REQUIRE(panlid::utils::Lerp(0, 10, 0.5) == 5);
    REQUIRE(panlid::utils::Lerp(0, 10, 0) == 0);
    REQUIRE(panlid::utils::Lerp(0, 10, 1) == 10);
}

TEST_CASE("ContinuousDamp", "[Interpolation]") {
    constexpr double kStart = 1;
    constexpr double kTarget = 2;
    constexpr double kHalfTime = 1000;

    auto damp = [](double current, double elapsed) {
        return panlid::utils::DampContinuously(current, kTarget, kHalfTime, elapsed);
    };

    REQUIRE(damp(kStart, 0) == kStart);
    REQUIRE(damp(kStart, std::numeric_limits<double>::infinity()) == kTarget);

    double middle = panlid::utils::Lerp(kStart, kTarget, 0.5);
    REQUIRE(damp(kStart, kHalfTime) == middle);

    REQUIRE(damp(kStart, 300 + 700) == damp(damp(kStart, 300), 700));
    REQUIRE(damp(kStart, 500 - 200) == damp(damp(kStart, 500), -200));
}

TEST_CASE("Lagrange", "[Interpolation]") {
    std::vector<glm::vec2> points{{0, 0}, {0.5F, 0.35F}, {1, 1}};

    for (double t = -10; t <= 10; t += 0.01) {
        CAPTURE(t);
        CHECK_THAT(panlid::utils::Lagrange(points, t), WithinAbs(0.6 * t * t + 0.4 * t, 1e-4));
    }

    std::vector<glm::vec2> empty;
    REQUIRE_THROWS_AS((void)panlid::utils::Lagrange(empty, 4), std::invalid_argument);

    std::vector<glm::vec2> singlePoint{{3, 4}};
    REQUIRE(panlid::utils::Lagrange(singlePoint, 12) == 4);

    std::vector<double> weights = panlid::utils::BarycentricWeights(points);
    REQUIRE(weights.size() == 3);
    REQUIRE(weights[0] == 2);
    REQUIRE(weights[1] == -4);
    REQUIRE(weights[2] == 2);
}

TEST_CASE("GenericInterpolation", "[Interpolation]") {
    REQUIRE(panlid::utils::ValueAt<int>(0.1, 0, 100, 0, 1) == 10);
    CHECK_THAT(panlid::utils::ValueAt<double>(0.1, 0.0, 0.1, 0, 1), WithinAbs(0.01, 1e-9));

    REQUIRE(panlid::utils::ValueAt(0.1, panlid::graphics::MarginPadding::FromAllSides(0),
                                   panlid::graphics::MarginPadding::FromAllSides(100), 0,
                                   1) == panlid::graphics::MarginPadding::FromAllSides(10));

    REQUIRE(panlid::utils::ValueAt(10.0, TestClassWithValueAt(0), TestClassWithValueAt(100), 0,
                                   20) == TestClassWithValueAt(50));
}
