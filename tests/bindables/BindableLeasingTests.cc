#include "panlid/bindables/Bindable.hh"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <stdexcept>

using namespace panlid::bindables;

TEST_CASE("LeaseAndReturn", "[Bindable][Leasing]") {
    bool revert = GENERATE(false, true);
    auto original = Bindable<int>::Create(1);
    auto leased = original->BeginLease(revert);

    REQUIRE(original->GetValue() == leased->GetValue());

    leased->SetValue(2);
    REQUIRE(original->GetValue() == 2);
    REQUIRE(original->GetValue() == leased->GetValue());

    REQUIRE(leased->Return());
    REQUIRE(original->GetValue() == (revert ? 1 : 2));
}

TEST_CASE("LeaseReturnedOnUnbindAll", "[Bindable][Leasing]") {
    bool revert = GENERATE(false, true);
    auto original = Bindable<int>::Create(1);
    auto leased = original->BeginLease(revert);

    leased->SetValue(2);
    original->UnbindAll();

    REQUIRE(original->GetValue() == (revert ? 1 : 2));
}

TEST_CASE("ConsecutiveLeases", "[Bindable][Leasing]") {
    auto original = Bindable<int>::Create(1);

    auto leased1 = original->BeginLease(false);
    REQUIRE(leased1->Return());
    auto leased2 = original->BeginLease(false);
    REQUIRE(leased2->Return());
}

TEST_CASE("ModifyAfterReturnFails", "[Bindable][Leasing]") {
    auto original = Bindable<int>::Create(1);
    auto leased = original->BeginLease(false);
    REQUIRE(leased->Return());

    REQUIRE_THROWS_AS(leased->SetValue(2), std::runtime_error);
    REQUIRE_THROWS_AS(leased->SetDisabled(true), std::runtime_error);
}

TEST_CASE("DoubleReturnSilentlyNoops", "[Bindable][Leasing]") {
    auto original = Bindable<int>::Create(1);
    auto leased = original->BeginLease(false);

    REQUIRE(leased->Return());
    REQUIRE_FALSE(leased->Return());
}

TEST_CASE("DoubleLeaseFails", "[Bindable][Leasing]") {
    auto original = Bindable<int>::Create(1);
    (void)original->BeginLease(false);
    REQUIRE_THROWS_AS(original->BeginLease(false), std::runtime_error);
}

TEST_CASE("IncorrectEndLeaseFails", "[Bindable][Leasing]") {
    auto original = Bindable<int>::Create(1);

    REQUIRE_THROWS_AS(original->DETAILEndLease(nullptr), std::runtime_error);

    (void)original->BeginLease(true);
    auto otherLease = Bindable<int>::Create()->BeginLease(true);
    REQUIRE_THROWS_AS(original->DETAILEndLease(otherLease.get()), std::runtime_error);
}

TEST_CASE("CantLeaseFromLease", "[Bindable][Leasing]") {
    auto original = Bindable<int>::Create(1);
    auto leased = original->BeginLease(false);
    REQUIRE_THROWS_AS(leased->BeginLease(false), std::runtime_error);
}

TEST_CASE("UnbindAllReturnsLeaseIdempotently", "[Bindable][Leasing]") {
    auto original = Bindable<int>::Create(1);
    auto leased = original->BeginLease(true);
    leased->UnbindAll();
    leased->UnbindAll(); // must not throw/crash the second time
}
