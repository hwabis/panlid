#include "panlid/bindables/Bindable.hh"
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>

using namespace panlid::bindables;

namespace {

enum class TestEnum : std::uint8_t { First, Second };

} // namespace

TEST_CASE("ConstructorValueUsedAsDefaultValue", "[Bindable]") {
    REQUIRE(Bindable<int>::Create(10)->GetDefault() == 10);
}

TEST_CASE("ConstructorValueUsedAsInitialValue", "[Bindable]") {
    REQUIRE(Bindable<int>::Create(10)->GetValue() == 10);
}

TEST_CASE("ParseBindableOfSameType", "[Bindable][Parse]") {
    auto intBindable = Bindable<int>::Create();
    auto doubleBindable = Bindable<double>::Create();
    auto boolBindable = Bindable<bool>::Create();
    auto stringBindable = Bindable<std::string>::Create();

    intBindable->Parse(*Bindable<int>::Create(3));
    doubleBindable->Parse(*Bindable<double>::Create(2.5));
    boolBindable->Parse(*Bindable<bool>::Create(true));
    stringBindable->Parse(*Bindable<std::string>::Create("string value"));

    REQUIRE(intBindable->GetValue() == 3);
    REQUIRE(doubleBindable->GetValue() == 2.5);
    REQUIRE(boolBindable->GetValue() == true);
    REQUIRE(stringBindable->GetValue() == "string value");

    REQUIRE_THROWS_AS(intBindable->Parse(*Bindable<double>::Create(3.0)), std::invalid_argument);
}

// todo TestParseBindableOfMatchingInterfaceType

TEMPLATE_TEST_CASE("ParseNumberFromText", "[Bindable][Parse]", std::int8_t, std::uint8_t,
                   std::int16_t, std::uint16_t, std::int32_t, std::uint32_t, std::int64_t,
                   std::uint64_t, float, double) {
    auto bindable = Bindable<TestType>::Create();

    bindable->Parse("1");
    REQUIRE(bindable->GetValue() == static_cast<TestType>(1));

    if constexpr (std::is_signed_v<TestType>) {
        bindable->Parse("-1");
        REQUIRE(bindable->GetValue() == static_cast<TestType>(-1));
    } else {
        REQUIRE_THROWS_AS(bindable->Parse("-1"), std::invalid_argument);
    }

    if constexpr (std::is_floating_point_v<TestType>) {
        bindable->Parse("1.5");
        REQUIRE(bindable->GetValue() == static_cast<TestType>(1.5));
        bindable->Parse("-1.5");
        REQUIRE(bindable->GetValue() == static_cast<TestType>(-1.5));
    } else {
        REQUIRE_THROWS_AS(bindable->Parse("1.5"), std::invalid_argument);
    }
}

TEST_CASE("ParseEmptyStringIntoValueType", "[Bindable][Parse]") {
    auto bindable = Bindable<int>::Create();
    REQUIRE_THROWS_AS(bindable->Parse(""), std::invalid_argument);
}

TEST_CASE("ParseOverflow", "[Bindable][Parse]") {
    auto bindable = Bindable<std::int8_t>::Create();
    REQUIRE_THROWS_AS(bindable->Parse("200"), std::out_of_range);
}

TEST_CASE("ParseBool", "[Bindable][Parse]") {
    auto bindable = Bindable<bool>::Create();
    bindable->Parse("True");
    REQUIRE(bindable->GetValue());
    bindable->Parse("false");
    REQUIRE_FALSE(bindable->GetValue());
    REQUIRE_THROWS_AS(bindable->Parse("1"), std::invalid_argument);
}

TEST_CASE("ParseEnum", "[Bindable][Parse]") {
    auto bindable = Bindable<TestEnum>::Create();
    bindable->Parse("Second");
    REQUIRE(bindable->GetValue() == TestEnum::Second);
    REQUIRE_THROWS_AS(bindable->Parse("second"), std::invalid_argument);
    REQUIRE_THROWS_AS(bindable->Parse("Third"), std::invalid_argument);
}

TEST_CASE("ParseStringIntoStringType", "[Bindable][Parse]") {
    auto bindable = Bindable<std::string>::Create("x");
    bindable->Parse("");
    REQUIRE(bindable->GetValue().empty());
}
