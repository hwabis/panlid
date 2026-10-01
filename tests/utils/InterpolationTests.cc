#include "panlid/graphics/Easing.hh"
#include "panlid/graphics/MarginPadding.hh"
#include "panlid/graphics/transforms/EasingFunction.hh"
#include "panlid/utils/Interpolation.hh"
#include "panlid/utils/Precision.hh"
#include <glm/vec2.hpp>
#include <gtest/gtest.h>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {

struct TestClassWithValueAt {
    int I{};
    constexpr explicit TestClassWithValueAt(int i) : I(i) {}
    constexpr bool operator==(const TestClassWithValueAt& other) const = default;
};

template <panlid::graphics::transforms::EasingFunction TEasing>
[[nodiscard]] TestClassWithValueAt ValueAt(double time, const TestClassWithValueAt& startValue,
                                           const TestClassWithValueAt& endValue, double startTime,
                                           double endTime, const TEasing& easing) {
    return TestClassWithValueAt(
        panlid::utils::ValueAt(time, startValue.I, endValue.I, startTime, endTime, easing));
}

} // namespace

TEST(Interpolation, EasingStartsAtZero) {
    for (auto easing : panlid::graphics::kAllEasings) {
        EXPECT_NEAR(panlid::utils::ApplyEasing(easing, 0), 0, panlid::utils::kDoubleEpsilon)
            << "easing index " << static_cast<int>(easing);
    }
}

TEST(Interpolation, EasingEndsAtOne) {
    for (auto easing : panlid::graphics::kAllEasings) {
        EXPECT_NEAR(panlid::utils::ApplyEasing(easing, 1), 1, panlid::utils::kDoubleEpsilon)
            << "easing index " << static_cast<int>(easing);
    }
}

TEST(Interpolation, Lerp) {
    EXPECT_DOUBLE_EQ(panlid::utils::Lerp(0, 10, 0.5), 5);
    EXPECT_DOUBLE_EQ(panlid::utils::Lerp(0, 10, 0), 0);
    EXPECT_DOUBLE_EQ(panlid::utils::Lerp(0, 10, 1), 10);
}

TEST(Interpolation, ContinuousDamp) {
    constexpr double kStart = 1;
    constexpr double kTarget = 2;
    constexpr double kHalfTime = 1000;

    auto damp = [](double current, double elapsed) {
        return panlid::utils::DampContinuously(current, kTarget, kHalfTime, elapsed);
    };

    EXPECT_DOUBLE_EQ(damp(kStart, 0), kStart);
    EXPECT_DOUBLE_EQ(damp(kStart, std::numeric_limits<double>::infinity()), kTarget);

    double middle = panlid::utils::Lerp(kStart, kTarget, 0.5);
    EXPECT_DOUBLE_EQ(damp(kStart, kHalfTime), middle);

    EXPECT_DOUBLE_EQ(damp(kStart, 300 + 700), damp(damp(kStart, 300), 700));

    EXPECT_DOUBLE_EQ(damp(kStart, 500 - 200), damp(damp(kStart, 500), -200));
}

TEST(Interpolation, Lagrange) {
    std::vector<glm::vec2> points{{0, 0}, {0.5F, 0.35F}, {1, 1}};

    for (double t = -10; t <= 10; t += 0.01) {
        EXPECT_NEAR(0.6 * t * t + 0.4 * t, panlid::utils::Lagrange(points, t), 1e-4);
    }

    std::vector<glm::vec2> empty;
    EXPECT_THROW((void)panlid::utils::Lagrange(empty, 4), std::invalid_argument);

    std::vector<glm::vec2> singlePoint{{3, 4}};
    EXPECT_DOUBLE_EQ(panlid::utils::Lagrange(singlePoint, 12), 4);

    std::vector<double> weights = panlid::utils::BarycentricWeights(points);
    ASSERT_EQ(weights.size(), 3U);
    EXPECT_DOUBLE_EQ(weights[0], 2);
    EXPECT_DOUBLE_EQ(weights[1], -4);
    EXPECT_DOUBLE_EQ(weights[2], 2);
}

TEST(Interpolation, GenericInterpolation) {
    EXPECT_EQ(panlid::utils::ValueAt<int>(0.1, 0, 100, 0, 1), 10);
    EXPECT_NEAR(panlid::utils::ValueAt<double>(0.1, 0.0, 0.1, 0, 1), 0.01, 1e-9);

    EXPECT_EQ(panlid::utils::ValueAt(0.1, panlid::graphics::MarginPadding(0),
                                     panlid::graphics::MarginPadding(100), 0, 1),
              panlid::graphics::MarginPadding(10));

    EXPECT_EQ(
        panlid::utils::ValueAt(10.0, TestClassWithValueAt(0), TestClassWithValueAt(100), 0, 20),
        TestClassWithValueAt(50));
}
