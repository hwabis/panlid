#include "panlid/graphics/Colour4.hh"
#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/io.hpp>
#include <stdexcept>
#include <string>

using panlid::graphics::Colour4;

namespace {

// note that we use a fairly high delta since the test constants are approximations
void assertAlmostEqual(const glm::vec4& expected, const glm::vec4& actual,
                       const char* type = "RGBA") {
    INFO(type);
    CAPTURE(expected, actual);
    REQUIRE(glm::all(glm::epsilonEqual(actual, expected, 0.005F)));
}

// 0x80 alpha is slightly more than half
constexpr float kHalfAlpha = 128.0F / 255.0F;

void testConvertFromHsl(const Colour4& expected, float h, float s, float l, float a) {
    assertAlmostEqual(expected.GetVector(), Colour4::FromHsl(h, s, l, a).GetVector());
}

void testConvertToHsl(const glm::vec4& expected, const Colour4& convert) {
    assertAlmostEqual(expected, convert.ToHsl(), "HSLA");
}

void testConvertFromHsv(const Colour4& expected, float h, float s, float v, float a) {
    assertAlmostEqual(expected.GetVector(), Colour4::FromHsv(h, s, v, a).GetVector());
}

void testConvertToHsv(const glm::vec4& expected, const Colour4& convert) {
    assertAlmostEqual(expected, convert.ToHsv(), "HSVA");
}

} // namespace

TEST_CASE("FromHSL", "[Colour4]") {
    // test FromHSL that black and white are only affected by luminance
    testConvertFromHsl(Colour4::GetWhite(), 0.0F, 0.5F, 1.0F, 1.0F);
    testConvertFromHsl(Colour4::GetWhite(), 1.0F, 1.0F, 1.0F, 1.0F);
    testConvertFromHsl(Colour4::GetWhite(), 0.5F, 0.75F, 1.0F, 1.0F);
    testConvertFromHsl(Colour4::GetBlack(), 0.0F, 0.5F, 0.0F, 1.0F);
    testConvertFromHsl(Colour4::GetBlack(), 1.0F, 1.0F, 0.0F, 1.0F);
    testConvertFromHsl(Colour4::GetBlack(), 0.5F, 0.75F, 0.0F, 1.0F);

    // test FromHSL that grey is not affected by hue
    testConvertFromHsl(Colour4::GetGray(), 0.0F, 0.0F, 0.5F, 1.0F);
    testConvertFromHsl(Colour4::GetGray(), 0.5F, 0.0F, 0.5F, 1.0F);
    testConvertFromHsl(Colour4::GetGray(), 1.0F, 0.0F, 0.5F, 1.0F);

    // test FromHSL that alpha is being passed through
    testConvertFromHsl(Colour4::GetBlack().Opacity(0.5F), 0.0F, 0.0F, 0.0F, 0.5F);

    // test FromHSL with primaries
    testConvertFromHsl(Colour4::GetRed(), 0.0F, 1.0F, 0.5F, 1.0F);
    testConvertFromHsl(Colour4::GetYellow(), 1.0F / 6.0F, 1.0F, 0.5F, 1.0F);
    testConvertFromHsl(Colour4::GetLime(), 2.0F / 6.0F, 1.0F, 0.5F, 1.0F);
    testConvertFromHsl(Colour4::GetCyan(), 3.0F / 6.0F, 1.0F, 0.5F, 1.0F);
    testConvertFromHsl(Colour4::GetBlue(), 4.0F / 6.0F, 1.0F, 0.5F, 1.0F);
    testConvertFromHsl(Colour4::GetMagenta(), 5.0F / 6.0F, 1.0F, 0.5F, 1.0F);
    testConvertFromHsl(Colour4::GetRed(), 1.0F, 1.0F, 0.5F, 1.0F);

    // test FromHSL with some other knowns
    testConvertFromHsl(Colour4::GetCornflowerBlue(), 219.0F / 360.0F, 0.792F, 0.661F, 1.0F);
    testConvertFromHsl(Colour4::GetTan().Opacity(0.5F), 34.0F / 360.0F, 0.437F, 0.686F, 0.5F);
}

TEST_CASE("ToHSL", "[Colour4]") {
    // test ToHSL that black, white, and grey always return constant 0f for hue and saturation
    testConvertToHsl({0.0F, 0.0F, 1.0F, 1.0F}, Colour4::GetWhite());
    testConvertToHsl({0.0F, 0.0F, 0.0F, 1.0F}, Colour4::GetBlack());
    testConvertToHsl({0.0F, 0.0F, 0.5F, 1.0F}, Colour4::GetGray());

    // test ToHSL that alpha is being passed through
    testConvertToHsl({0.0F, 0.0F, 0.0F, 0.5F}, Colour4::GetBlack().Opacity(0.5F));

    // test ToHSL with primaries
    testConvertToHsl({0.0F, 1.0F, 0.5F, 1.0F}, Colour4::GetRed());
    testConvertToHsl({1.0F / 6.0F, 1.0F, 0.5F, 1.0F}, Colour4::GetYellow());
    testConvertToHsl({2.0F / 6.0F, 1.0F, 0.5F, 1.0F}, Colour4::GetLime());
    testConvertToHsl({3.0F / 6.0F, 1.0F, 0.5F, 1.0F}, Colour4::GetCyan());
    testConvertToHsl({4.0F / 6.0F, 1.0F, 0.5F, 1.0F}, Colour4::GetBlue());
    testConvertToHsl({5.0F / 6.0F, 1.0F, 0.5F, 1.0F}, Colour4::GetMagenta());

    // test ToHSL with some other knowns
    testConvertToHsl({219.0F / 360.0F, 0.792F, 0.661F, 1.0F}, Colour4::GetCornflowerBlue());
    testConvertToHsl({34.0F / 360.0F, 0.437F, 0.686F, 0.5F}, Colour4::GetTan().Opacity(0.5F));
}

TEST_CASE("FromHSV", "[Colour4]") {
    // test FromHSV that black is only affected by luminance
    testConvertFromHsv(Colour4::GetBlack(), 0.0F, 0.5F, 0.0F, 1.0F);
    testConvertFromHsv(Colour4::GetBlack(), 1.0F, 1.0F, 0.0F, 1.0F);
    testConvertFromHsv(Colour4::GetBlack(), 0.5F, 0.75F, 0.0F, 1.0F);

    // test FromHSV that white and grey are not affected by hue
    testConvertFromHsv(Colour4::GetWhite(), 0.0F, 0.0F, 1.0F, 1.0F);
    testConvertFromHsv(Colour4::GetWhite(), 1.0F, 0.0F, 1.0F, 1.0F);
    testConvertFromHsv(Colour4::GetWhite(), 0.5F, 0.0F, 1.0F, 1.0F);
    testConvertFromHsv(Colour4::GetGray(), 0.0F, 0.0F, 0.5F, 1.0F);
    testConvertFromHsv(Colour4::GetGray(), 0.5F, 0.0F, 0.5F, 1.0F);
    testConvertFromHsv(Colour4::GetGray(), 1.0F, 0.0F, 0.5F, 1.0F);

    // test FromHSV that alpha is being passed through
    testConvertFromHsv(Colour4::GetBlack().Opacity(0.5F), 0.0F, 0.0F, 0.0F, 0.5F);

    // test FromHSV with primaries
    testConvertFromHsv(Colour4::GetRed(), 0.0F, 1.0F, 1.0F, 1.0F);
    testConvertFromHsv(Colour4::GetYellow(), 1.0F / 6.0F, 1.0F, 1.0F, 1.0F);
    testConvertFromHsv(Colour4::GetLime(), 2.0F / 6.0F, 1.0F, 1.0F, 1.0F);
    testConvertFromHsv(Colour4::GetCyan(), 3.0F / 6.0F, 1.0F, 1.0F, 1.0F);
    testConvertFromHsv(Colour4::GetBlue(), 4.0F / 6.0F, 1.0F, 1.0F, 1.0F);
    testConvertFromHsv(Colour4::GetMagenta(), 5.0F / 6.0F, 1.0F, 1.0F, 1.0F);
    testConvertFromHsv(Colour4::GetRed(), 1.0F, 1.0F, 1.0F, 1.0F);

    // test FromHSV with some other knowns
    testConvertFromHsv(Colour4::GetCornflowerBlue(), 219.0F / 360.0F, 0.578F, 0.929F, 1.0F);
    testConvertFromHsv(Colour4::GetTan().Opacity(0.5F), 34.0F / 360.0F, 0.333F, 0.824F, 0.5F);
}

TEST_CASE("ToHSV", "[Colour4]") {
    // test ToHSV that black, white, and grey always return constant 0f for hue and saturation
    testConvertToHsv({0.0F, 0.0F, 1.0F, 1.0F}, Colour4::GetWhite());
    testConvertToHsv({0.0F, 0.0F, 0.0F, 1.0F}, Colour4::GetBlack());
    testConvertToHsv({0.0F, 0.0F, 0.5F, 1.0F}, Colour4::GetGray());

    // test ToHSV that alpha is being passed through
    testConvertToHsv({0.0F, 0.0F, 1.0F, 0.5F}, Colour4::GetWhite().Opacity(0.5F));

    // test ToHSV with primaries
    testConvertToHsv({0.0F, 1.0F, 1.0F, 1.0F}, Colour4::GetRed());
    testConvertToHsv({1.0F / 6.0F, 1.0F, 1.0F, 1.0F}, Colour4::GetYellow());
    testConvertToHsv({2.0F / 6.0F, 1.0F, 1.0F, 1.0F}, Colour4::GetLime());
    testConvertToHsv({3.0F / 6.0F, 1.0F, 1.0F, 1.0F}, Colour4::GetCyan());
    testConvertToHsv({4.0F / 6.0F, 1.0F, 1.0F, 1.0F}, Colour4::GetBlue());
    testConvertToHsv({5.0F / 6.0F, 1.0F, 1.0F, 1.0F}, Colour4::GetMagenta());

    // test ToHSV with some other knowns
    testConvertToHsv({219.0F / 360.0F, 0.578F, 0.929F, 1.0F}, Colour4::GetCornflowerBlue());
    testConvertToHsv({34.0F / 360.0F, 0.333F, 0.824F, 0.5F}, Colour4::GetTan().Opacity(0.5F));
}

TEST_CASE("ToHex", "[Colour4]") {
    REQUIRE(Colour4::GetTan().ToHex() == "#D2B48C");
    REQUIRE(Colour4::GetTan().ToHex(true) == "#D2B48CFF");
    REQUIRE(Colour4::GetCornflowerBlue().Opacity(kHalfAlpha).ToHex() == "#6495ED80");
}

TEST_CASE("FromHex", "[Colour4]") {
    const auto [expectedColour, hexCode] = GENERATE(table<Colour4, std::string>({
        {Colour4::GetWhite(), "#fff"},
        {Colour4::GetRed(), "#ff0000"},
        {Colour4::GetYellow().Opacity(kHalfAlpha), "ffff0080"},
        {Colour4::GetLime().Opacity(kHalfAlpha), "00ff0080"},
        {Colour4::FromBytes(17, 34, 51, 255), "123"},
        {Colour4::FromBytes(17, 34, 51, 255), "#123"},
        {Colour4::FromBytes(17, 34, 51, 68), "1234"},
        {Colour4::FromBytes(17, 34, 51, 68), "#1234"},
        {Colour4::FromBytes(18, 52, 86, 255), "123456"},
        {Colour4::FromBytes(18, 52, 86, 255), "#123456"},
        {Colour4::FromBytes(18, 52, 86, 120), "12345678"},
        {Colour4::FromBytes(18, 52, 86, 120), "#12345678"},
    }));
    INFO("hex code: " << hexCode);

    REQUIRE(Colour4::FromHex(hexCode) == expectedColour);

    const auto actualColour = Colour4::TryParseHex(hexCode);
    REQUIRE(actualColour.has_value());
    REQUIRE(*actualColour == expectedColour);
}

TEST_CASE("FromHex fails on invalid colours", "[Colour4]") {
    const auto invalidColour =
        GENERATE(as<std::string>{}, "1", "#1", "12", "#12", "12345", "#12345", "1234567",
                 "#1234567", "123456789", "#123456789", "gg00zz");
    INFO("hex code: " << invalidColour);

    REQUIRE_THROWS(Colour4::FromHex(invalidColour));
    REQUIRE_FALSE(Colour4::TryParseHex(invalidColour).has_value());
}

TEST_CASE("Chaining functions", "[Colour4]") {
    // test that Opacity replaces alpha channel rather than multiplying
    const Colour4 expected1(1.0F, 0.0F, 0.0F, 0.5F);
    REQUIRE(Colour4::GetRed().Opacity(0.5F) == expected1);
    REQUIRE(expected1.Opacity(0.5F) == expected1);

    // test that MultiplyAlpha multiplies existing alpha channel
    const Colour4 expected2(1.0F, 0.0F, 0.0F, 0.25F);
    REQUIRE(expected1.MultiplyAlpha(0.5F) == expected2);
    REQUIRE_THROWS_AS(Colour4::GetWhite().MultiplyAlpha(-1.0F), std::invalid_argument);

    // test clamping all channels in either direction
    REQUIRE(Colour4(1.1F, 1.1F, 1.1F, 1.1F).Clamped() == Colour4::GetWhite());
    REQUIRE(Colour4(-1.1F, -1.1F, -1.1F, -1.1F).Clamped() == Colour4::GetBlack().Opacity(0.0F));

    // test lighten and darken
    assertAlmostEqual(Colour4(0.431F, 0.642F, 1.0F, 1.0F).GetVector(),
                      Colour4::GetCornflowerBlue().Lighten(0.1F).GetVector());
    assertAlmostEqual(Colour4(0.356F, 0.531F, 0.845F, 1.0F).GetVector(),
                      Colour4::GetCornflowerBlue().Darken(0.1F).GetVector());
}

TEST_CASE("Operators", "[Colour4]") {
    const Colour4 colour(0.5F, 0.5F, 0.5F, 0.5F);
    assertAlmostEqual({0.6F, 0.7F, 0.8F, 0.9F},
                      (colour + Colour4(0.1F, 0.2F, 0.3F, 0.4F)).GetVector());
    assertAlmostEqual({0.4F, 0.3F, 0.2F, 0.1F},
                      (colour - Colour4(0.1F, 0.2F, 0.3F, 0.4F)).GetVector());
    assertAlmostEqual({0.25F, 0.25F, 0.25F, 0.25F}, (colour * colour).GetVector());
    assertAlmostEqual({0.25F, 0.25F, 0.25F, 0.25F}, (colour / 2.0F).GetVector());
    assertAlmostEqual(Colour4::GetWhite().GetVector(), (colour * 2.0F).GetVector());
    REQUIRE_THROWS_AS(colour * -1.0F, std::invalid_argument);
    REQUIRE_THROWS_AS(colour / -1.0F, std::invalid_argument);
    REQUIRE_THROWS_AS(colour / 0.0F, std::invalid_argument);
}

TEST_CASE("Other conversions", "[Colour4]") {
    // test uint conversions
    REQUIRE(Colour4::GetCornflowerBlue().Opacity(kHalfAlpha).ToRgba() == 0x6495ED80U);
    REQUIRE(Colour4::GetCornflowerBlue().Opacity(kHalfAlpha).ToArgb() == 0x806495EDU);
    REQUIRE(Colour4::FromRgba(0x6495ED80U) == Colour4::GetCornflowerBlue().Opacity(kHalfAlpha));
    REQUIRE(Colour4::FromArgb(0x806495EDU) == Colour4::GetCornflowerBlue().Opacity(kHalfAlpha));

    // test SRGB
    const glm::vec4 srgb(0.659F, 0.788F, 0.968F, 1.0F);
    assertAlmostEqual(srgb, Colour4::GetCornflowerBlue().ToSrgb().GetVector());
    assertAlmostEqual(Colour4::GetCornflowerBlue().GetVector(),
                      Colour4(srgb).ToLinear().GetVector());
}
