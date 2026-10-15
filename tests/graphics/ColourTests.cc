#include "panlid/graphics/Colour4.hh"
#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <format>
#include <glm/glm.hpp>
#include <stdexcept>
#include <string>
#include <string_view>

using panlid::graphics::Colour4;

namespace {

// 0x80 alpha is slightly more than half.
constexpr float kHalfAlpha = 128.F / 255.F;

void AssertAlmostEqual(const glm::vec4& expected, const glm::vec4& actual,
                       std::string_view type = "RGBA") {
    // The test constants are approximations, hence the fairly high delta.
    constexpr double kDelta = 0.005;

    INFO(std::format("({}) Expected: ({}, {}, {}, {}), Actual: ({}, {}, {}, {})", type, expected.x,
                     expected.y, expected.z, expected.w, actual.x, actual.y, actual.z, actual.w));
    CHECK_THAT(actual.x, Catch::Matchers::WithinAbs(expected.x, kDelta));
    CHECK_THAT(actual.y, Catch::Matchers::WithinAbs(expected.y, kDelta));
    CHECK_THAT(actual.z, Catch::Matchers::WithinAbs(expected.z, kDelta));
    CHECK_THAT(actual.w, Catch::Matchers::WithinAbs(expected.w, kDelta));
}

void ExpectFromHsl(const Colour4& expected, float hue, float saturation, float lightness,
                   float alpha) {
    AssertAlmostEqual(expected.GetVector(),
                      Colour4::FromHsl(hue, saturation, lightness, alpha).GetVector());
}

void ExpectToHsl(float hue, float saturation, float lightness, float alpha, const Colour4& colour) {
    AssertAlmostEqual({hue, saturation, lightness, alpha}, colour.ToHsl(), "HSLA");
}

void ExpectFromHsv(const Colour4& expected, float hue, float saturation, float value, float alpha) {
    AssertAlmostEqual(expected.GetVector(),
                      Colour4::FromHsv(hue, saturation, value, alpha).GetVector());
}

void ExpectToHsv(float hue, float saturation, float value, float alpha, const Colour4& colour) {
    AssertAlmostEqual({hue, saturation, value, alpha}, colour.ToHsv(), "HSVA");
}

} // namespace

TEST_CASE("Colour FromHsl", "[graphics][colour]") {
    // Black and white are only affected by lightness.
    ExpectFromHsl(Colour4::White(), 0.F, 0.5F, 1.F, 1.F);
    ExpectFromHsl(Colour4::White(), 1.F, 1.F, 1.F, 1.F);
    ExpectFromHsl(Colour4::White(), 0.5F, 0.75F, 1.F, 1.F);
    ExpectFromHsl(Colour4::Black(), 0.F, 0.5F, 0.F, 1.F);
    ExpectFromHsl(Colour4::Black(), 1.F, 1.F, 0.F, 1.F);
    ExpectFromHsl(Colour4::Black(), 0.5F, 0.75F, 0.F, 1.F);

    // Grey is not affected by hue.
    ExpectFromHsl(Colour4::Gray(), 0.F, 0.F, 0.5F, 1.F);
    ExpectFromHsl(Colour4::Gray(), 0.5F, 0.F, 0.5F, 1.F);
    ExpectFromHsl(Colour4::Gray(), 1.F, 0.F, 0.5F, 1.F);

    // Alpha is passed through.
    ExpectFromHsl(Colour4::Black().Opacity(0.5F), 0.F, 0.F, 0.F, 0.5F);

    // Primaries.
    ExpectFromHsl(Colour4::Red(), 0.F, 1.F, 0.5F, 1.F);
    ExpectFromHsl(Colour4::Yellow(), 1.F / 6.F, 1.F, 0.5F, 1.F);
    ExpectFromHsl(Colour4::Lime(), 2.F / 6.F, 1.F, 0.5F, 1.F);
    ExpectFromHsl(Colour4::Cyan(), 3.F / 6.F, 1.F, 0.5F, 1.F);
    ExpectFromHsl(Colour4::Blue(), 4.F / 6.F, 1.F, 0.5F, 1.F);
    ExpectFromHsl(Colour4::Magenta(), 5.F / 6.F, 1.F, 0.5F, 1.F);
    ExpectFromHsl(Colour4::Red(), 1.F, 1.F, 0.5F, 1.F);

    // Some other known values.
    ExpectFromHsl(Colour4::CornflowerBlue(), 219.F / 360.F, 0.792F, 0.661F, 1.F);
    ExpectFromHsl(Colour4::Tan().Opacity(0.5F), 34.F / 360.F, 0.437F, 0.686F, 0.5F);
}

TEST_CASE("Colour ToHsl", "[graphics][colour]") {
    // Black, white and grey always return 0 for hue and saturation.
    ExpectToHsl(0.F, 0.F, 1.F, 1.F, Colour4::White());
    ExpectToHsl(0.F, 0.F, 0.F, 1.F, Colour4::Black());
    ExpectToHsl(0.F, 0.F, 0.5F, 1.F, Colour4::Gray());

    // Alpha is passed through.
    ExpectToHsl(0.F, 0.F, 0.F, 0.5F, Colour4::Black().Opacity(0.5F));

    // Primaries.
    ExpectToHsl(0.F, 1.F, 0.5F, 1.F, Colour4::Red());
    ExpectToHsl(1.F / 6.F, 1.F, 0.5F, 1.F, Colour4::Yellow());
    ExpectToHsl(2.F / 6.F, 1.F, 0.5F, 1.F, Colour4::Lime());
    ExpectToHsl(3.F / 6.F, 1.F, 0.5F, 1.F, Colour4::Cyan());
    ExpectToHsl(4.F / 6.F, 1.F, 0.5F, 1.F, Colour4::Blue());
    ExpectToHsl(5.F / 6.F, 1.F, 0.5F, 1.F, Colour4::Magenta());

    // Some other known values.
    ExpectToHsl(219.F / 360.F, 0.792F, 0.661F, 1.F, Colour4::CornflowerBlue());
    ExpectToHsl(34.F / 360.F, 0.437F, 0.686F, 0.5F, Colour4::Tan().Opacity(0.5F));
}

TEST_CASE("Colour FromHsv", "[graphics][colour]") {
    // Black is only affected by value.
    ExpectFromHsv(Colour4::Black(), 0.F, 0.5F, 0.F, 1.F);
    ExpectFromHsv(Colour4::Black(), 1.F, 1.F, 0.F, 1.F);
    ExpectFromHsv(Colour4::Black(), 0.5F, 0.75F, 0.F, 1.F);

    // White and grey are not affected by hue.
    ExpectFromHsv(Colour4::White(), 0.F, 0.F, 1.F, 1.F);
    ExpectFromHsv(Colour4::White(), 1.F, 0.F, 1.F, 1.F);
    ExpectFromHsv(Colour4::White(), 0.5F, 0.F, 1.F, 1.F);
    ExpectFromHsv(Colour4::Gray(), 0.F, 0.F, 0.5F, 1.F);
    ExpectFromHsv(Colour4::Gray(), 0.5F, 0.F, 0.5F, 1.F);
    ExpectFromHsv(Colour4::Gray(), 1.F, 0.F, 0.5F, 1.F);

    // Alpha is passed through.
    ExpectFromHsv(Colour4::Black().Opacity(0.5F), 0.F, 0.F, 0.F, 0.5F);

    // Primaries.
    ExpectFromHsv(Colour4::Red(), 0.F, 1.F, 1.F, 1.F);
    ExpectFromHsv(Colour4::Yellow(), 1.F / 6.F, 1.F, 1.F, 1.F);
    ExpectFromHsv(Colour4::Lime(), 2.F / 6.F, 1.F, 1.F, 1.F);
    ExpectFromHsv(Colour4::Cyan(), 3.F / 6.F, 1.F, 1.F, 1.F);
    ExpectFromHsv(Colour4::Blue(), 4.F / 6.F, 1.F, 1.F, 1.F);
    ExpectFromHsv(Colour4::Magenta(), 5.F / 6.F, 1.F, 1.F, 1.F);
    ExpectFromHsv(Colour4::Red(), 1.F, 1.F, 1.F, 1.F);

    // Some other known values.
    ExpectFromHsv(Colour4::CornflowerBlue(), 219.F / 360.F, 0.578F, 0.929F, 1.F);
    ExpectFromHsv(Colour4::Tan().Opacity(0.5F), 34.F / 360.F, 0.333F, 0.824F, 0.5F);
}

TEST_CASE("Colour ToHsv", "[graphics][colour]") {
    // Black, white and grey always return 0 for hue and saturation.
    ExpectToHsv(0.F, 0.F, 1.F, 1.F, Colour4::White());
    ExpectToHsv(0.F, 0.F, 0.F, 1.F, Colour4::Black());
    ExpectToHsv(0.F, 0.F, 0.5F, 1.F, Colour4::Gray());

    // Alpha is passed through.
    ExpectToHsv(0.F, 0.F, 1.F, 0.5F, Colour4::White().Opacity(0.5F));

    // Primaries.
    ExpectToHsv(0.F, 1.F, 1.F, 1.F, Colour4::Red());
    ExpectToHsv(1.F / 6.F, 1.F, 1.F, 1.F, Colour4::Yellow());
    ExpectToHsv(2.F / 6.F, 1.F, 1.F, 1.F, Colour4::Lime());
    ExpectToHsv(3.F / 6.F, 1.F, 1.F, 1.F, Colour4::Cyan());
    ExpectToHsv(4.F / 6.F, 1.F, 1.F, 1.F, Colour4::Blue());
    ExpectToHsv(5.F / 6.F, 1.F, 1.F, 1.F, Colour4::Magenta());

    // Some other known values.
    ExpectToHsv(219.F / 360.F, 0.578F, 0.929F, 1.F, Colour4::CornflowerBlue());
    ExpectToHsv(34.F / 360.F, 0.333F, 0.824F, 0.5F, Colour4::Tan().Opacity(0.5F));
}

TEST_CASE("Colour ToHex", "[graphics][colour]") {
    CHECK(Colour4::Tan().ToHex() == "#D2B48C");
    CHECK(Colour4::Tan().ToHex(true) == "#D2B48CFF");
    CHECK(Colour4::CornflowerBlue().Opacity(kHalfAlpha).ToHex() == "#6495ED80");
}

TEST_CASE("Colour FromHex", "[graphics][colour]") {
    const auto [expected, hex] = GENERATE(table<Colour4, std::string>({
        {Colour4::White(), "#fff"},
        {Colour4::Red(), "#ff0000"},
        {Colour4::Yellow().Opacity(kHalfAlpha), "ffff0080"},
        {Colour4::Lime().Opacity(kHalfAlpha), "00ff0080"},
        {Colour4::FromBytes(17, 34, 51, 255), "123"},
        {Colour4::FromBytes(17, 34, 51, 255), "#123"},
        {Colour4::FromBytes(17, 34, 51, 68), "1234"},
        {Colour4::FromBytes(17, 34, 51, 68), "#1234"},
        {Colour4::FromBytes(18, 52, 86, 255), "123456"},
        {Colour4::FromBytes(18, 52, 86, 255), "#123456"},
        {Colour4::FromBytes(18, 52, 86, 120), "12345678"},
        {Colour4::FromBytes(18, 52, 86, 120), "#12345678"},
    }));

    CHECK(Colour4::FromHex(hex) == expected);

    const auto parsed = Colour4::TryParseHex(hex);
    REQUIRE(parsed.has_value());
    CHECK(*parsed == expected);
}

TEST_CASE("Colour FromHex fails on invalid colours", "[graphics][colour]") {
    const auto invalid = GENERATE(as<std::string>{}, "1", "#1", "12", "#12", "12345", "#12345",
                                  "1234567", "#1234567", "123456789", "#123456789", "gg00zz");

    CHECK_THROWS(Colour4::FromHex(invalid));
    CHECK_FALSE(Colour4::TryParseHex(invalid).has_value());
}

TEST_CASE("Colour chaining functions", "[graphics][colour]") {
    // Opacity replaces the alpha channel rather than multiplying.
    const Colour4 expected1{.R = 1.F, .G = 0.F, .B = 0.F, .A = 0.5F};
    CHECK(Colour4::Red().Opacity(0.5F) == expected1);
    CHECK(expected1.Opacity(0.5F) == expected1);

    // MultiplyAlpha multiplies the existing alpha channel.
    const Colour4 expected2{.R = 1.F, .G = 0.F, .B = 0.F, .A = 0.25F};
    CHECK(expected1.MultiplyAlpha(0.5F) == expected2);
    CHECK_THROWS_AS(Colour4::White().MultiplyAlpha(-1.F), std::invalid_argument);

    // Clamping all channels in either direction.
    CHECK(Colour4{.R = 1.1F, .G = 1.1F, .B = 1.1F, .A = 1.1F}.Clamped() == Colour4::White());
    CHECK(Colour4{.R = -1.1F, .G = -1.1F, .B = -1.1F, .A = -1.1F}.Clamped() ==
          Colour4::Black().Opacity(0.F));

    // Lighten and darken.
    AssertAlmostEqual(Colour4{.R = 0.431F, .G = 0.642F, .B = 1.F, .A = 1.F}.GetVector(),
                      Colour4::CornflowerBlue().Lighten(0.1F).GetVector());
    AssertAlmostEqual(Colour4{.R = 0.356F, .G = 0.531F, .B = 0.845F, .A = 1.F}.GetVector(),
                      Colour4::CornflowerBlue().Darken(0.1F).GetVector());
}

TEST_CASE("Colour operators", "[graphics][colour]") {
    const Colour4 colour{.R = 0.5F, .G = 0.5F, .B = 0.5F, .A = 0.5F};
    const Colour4 other{.R = 0.1F, .G = 0.2F, .B = 0.3F, .A = 0.4F};

    AssertAlmostEqual({0.6F, 0.7F, 0.8F, 0.9F}, (colour + other).GetVector());
    AssertAlmostEqual({0.4F, 0.3F, 0.2F, 0.1F}, (colour - other).GetVector());
    AssertAlmostEqual({0.25F, 0.25F, 0.25F, 0.25F}, (colour * colour).GetVector());
    AssertAlmostEqual({0.25F, 0.25F, 0.25F, 0.25F}, (colour / 2.F).GetVector());
    AssertAlmostEqual(Colour4::White().GetVector(), (colour * 2.F).GetVector());

    CHECK_THROWS_AS(colour * -1.F, std::invalid_argument);
    CHECK_THROWS_AS(colour / -1.F, std::invalid_argument);
    CHECK_THROWS_AS(colour / 0.F, std::invalid_argument);
}

TEST_CASE("Colour other conversions", "[graphics][colour]") {
    const Colour4 halfAlphaCornflower = Colour4::CornflowerBlue().Opacity(kHalfAlpha);

    // uint conversions.
    CHECK(halfAlphaCornflower.ToRgba() == 0x6495ED80U);
    CHECK(halfAlphaCornflower.ToArgb() == 0x806495EDU);
    CHECK(Colour4::FromRgba(0x6495ED80U) == halfAlphaCornflower);
    CHECK(Colour4::FromArgb(0x806495EDU) == halfAlphaCornflower);

    // sRGB.
    const glm::vec4 srgb{0.659F, 0.788F, 0.968F, 1.F};
    AssertAlmostEqual(srgb, Colour4::CornflowerBlue().ToSrgb().GetVector());
    AssertAlmostEqual(Colour4::CornflowerBlue().GetVector(),
                      Colour4::FromVector(srgb).ToLinear().GetVector());
}
