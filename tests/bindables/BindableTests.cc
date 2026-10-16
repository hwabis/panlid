#include "panlid/bindables/Bindable.hh"
#include <array>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

using namespace panlid::bindables;

namespace {

struct TestClass {};

struct ParseInput {
    std::string_view Text;
    double Value;
};

constexpr std::array kParseInputs{
    ParseInput{.Text = "1", .Value = 1.0}, ParseInput{.Text = "1.5", .Value = 1.5},
    ParseInput{.Text = "-1", .Value = -1.0}, ParseInput{.Text = "-1.5", .Value = -1.5}};

template <typename T> bool ExpectsParseSuccess(const ParseInput& input) {
    if constexpr (std::is_floating_point_v<T> || std::is_same_v<T, std::string>) {
        return true;
    } else {
        const bool isWhole = input.Value == std::trunc(input.Value);
        return isWhole && (input.Value >= 0 || std::is_signed_v<T>);
    }
}

template <typename T> T ExpectedParseValue(const ParseInput& input) {
    if constexpr (std::is_same_v<T, std::string>) {
        return std::string{input.Text};
    } else {
        return static_cast<T>(input.Value);
    }
}

template <typename T> void CheckParse(const ParseInput& input) {
    auto bindable = Bindable<T>::Create();

    if (!ExpectsParseSuccess<T>(input)) {
        REQUIRE_THROWS_AS(bindable->Parse(input.Text), std::invalid_argument);
        return;
    }

    bindable->Parse(input.Text);
    REQUIRE(bindable->GetValue() == ExpectedParseValue<T>(input));
}

} // namespace

TEST_CASE("ConstructorValueUsedAsDefaultValue", "[Bindable]") {
    REQUIRE(Bindable<int>::Create(10)->GetDefault() == 10);
}

TEST_CASE("ConstructorValueUsedAsInitialValue", "[Bindable]") {
    REQUIRE(Bindable<int>::Create(10)->GetValue() == 10);
}

TEST_CASE("ParseBindableOfExactSameType", "[Bindable]") {
    auto bindable1 = Bindable<int>::Create();
    auto bindable2 = Bindable<double>::Create();
    auto bindable3 = Bindable<bool>::Create();
    auto bindable4 = Bindable<std::string>::Create();

    bindable1->Parse(*Bindable<int>::Create(3));
    bindable2->Parse(*Bindable<double>::Create(2.5));
    bindable3->Parse(*Bindable<bool>::Create(true));
    bindable4->Parse(*Bindable<std::string>::Create("string value"));

    REQUIRE(bindable1->GetValue() == 3);
    REQUIRE(bindable2->GetValue() == 2.5);
    REQUIRE(bindable3->GetValue());
    REQUIRE(bindable4->GetValue() == "string value");

    REQUIRE_THROWS_AS(bindable1->Parse(*Bindable<double>::Create(3.0)), std::invalid_argument);
}

TEMPLATE_TEST_CASE("Parse", "[Bindable]", short, unsigned short, int, unsigned int, long long,
                   unsigned long long, float, double, std::uint8_t, std::int8_t, std::string) {
    for (const auto& input : kParseInputs) {
        CheckParse<TestType>(input);
    }
}

TEST_CASE("ParseEmptyStringIntoValueType", "[Bindable]") {
    auto bindable = Bindable<int>::Create();
    REQUIRE_THROWS_AS(bindable->Parse(""), std::invalid_argument);
}

TEST_CASE("ParseEmptyStringIntoNullableValueType", "[Bindable]") {
    auto bindable = Bindable<std::optional<int>>::Create(5);
    bindable->Parse("");
    REQUIRE_FALSE(bindable->GetValue().has_value());
}

TEST_CASE("ParseEmptyStringIntoReferenceType", "[Bindable]") {
    auto bindable = Bindable<std::shared_ptr<TestClass>>::Create(std::make_shared<TestClass>());
    bindable->Parse("");
    REQUIRE(bindable->GetValue() == nullptr);
}

TEST_CASE("ParseEmptyStringIntoStringType", "[Bindable]") {
    auto bindable = Bindable<std::string>::Create("not empty");
    bindable->Parse("");
    REQUIRE(bindable->GetValue().empty());
}
