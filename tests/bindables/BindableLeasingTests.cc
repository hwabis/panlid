#include "panlid/bindables/Bindable.hh"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <memory>
#include <optional>
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

    REQUIRE_THROWS_AS(original->EndLease(nullptr), std::runtime_error);

    (void)original->BeginLease(true);
    auto otherLease = Bindable<int>::Create()->BeginLease(true);
    REQUIRE_THROWS_AS(original->EndLease(otherLease.get()), std::runtime_error);
}

TEST_CASE("DisabledStateDuringLease", "[Bindable][Leasing]") {
    auto original = Bindable<int>::Create(1);
    auto leased = original->BeginLease(true);

    REQUIRE(original->IsDisabled());
    // during lease, the leased bindable is also set to a disabled state (but is always bypassed
    // when setting the value via it directly).
    REQUIRE(leased->IsDisabled());

    // you can't change the disabled state of the original during a lease...
    REQUIRE_THROWS_AS(original->SetDisabled(false), std::runtime_error);

    // ..but you can change it from the leased instance..
    leased->SetDisabled(false);

    REQUIRE_FALSE(leased->IsDisabled());
    REQUIRE_FALSE(original->IsDisabled());

    // ..allowing modification of the original during lease.
    original->SetValue(2);

    // even if not disabled, you still cannot change disabled from the original during a lease.
    REQUIRE_THROWS_AS(original->SetDisabled(true), std::runtime_error);
    REQUIRE_FALSE(original->IsDisabled());
    REQUIRE_FALSE(leased->IsDisabled());

    // you must use the leased instance.
    leased->SetDisabled(true);

    REQUIRE(original->IsDisabled());
    REQUIRE(leased->IsDisabled());

    REQUIRE(leased->Return());

    REQUIRE_FALSE(original->IsDisabled());
}

TEST_CASE("DisabledChangeViaBindings", "[Bindable][Leasing]") {
    auto original = Bindable<int>::Create(1);
    auto leased = original->BeginLease(true);
    // ensure we can't change original's disabled via a bound bindable.
    auto bound = original->GetBoundCopy();

    REQUIRE_THROWS_AS(bound->SetDisabled(false), std::runtime_error);
    REQUIRE(original->IsDisabled());
}

TEST_CASE("DisabledChangeViaBindingToLeased", "[Bindable][Leasing]") {
    auto original = Bindable<int>::Create(1);

    std::optional<bool> changedState;
    original->DisabledChanged += [&](bool d) { changedState = d; };

    auto leased = original->BeginLease(true);
    auto bound = leased->GetBoundCopy();

    bound->SetDisabled(false);

    REQUIRE(changedState.has_value());
    REQUIRE_FALSE(changedState.value());
    REQUIRE_FALSE(original->IsDisabled());
}

TEST_CASE("ValueChangeViaBindings", "[Bindable][Leasing]") {
    auto original = Bindable<int>::Create(1);
    auto leased = original->BeginLease(true);
    auto bound = original->GetBoundCopy();

    REQUIRE_THROWS_AS(bound->SetValue(2), std::runtime_error);
    REQUIRE(original->GetValue() == 1);
}

TEST_CASE("DisabledRevertedAfterLease", "[Bindable][Leasing]") {
    bool revert = GENERATE(false, true);
    auto original = Bindable<int>::Create(1);

    std::optional<bool> changedState;
    original->SetDisabled(true);
    original->DisabledChanged += [&](bool d) { changedState = d; };

    auto leased = original->BeginLease(revert);

    REQUIRE(leased->Return());

    // regardless of revert specification, disabled should always be reverted to the original value.
    REQUIRE(original->IsDisabled());
    REQUIRE_FALSE(changedState.has_value());
}

TEST_CASE("LeaseFromBoundBindable", "[Bindable][Leasing]") {
    auto original = Bindable<int>::Create(1);
    auto copy = original->GetBoundCopy();
    auto leased = original->BeginLease(true);

    // can't take a second lease from the original.
    REQUIRE_THROWS_AS(original->BeginLease(false), std::runtime_error);

    // can't take a second lease from the copy.
    REQUIRE_THROWS_AS(copy->BeginLease(false), std::runtime_error);

    leased->SetValue(2);

    // value propagates everywhere
    REQUIRE(original->GetValue() == 2);
    REQUIRE(original->GetValue() == copy->GetValue());
    REQUIRE(original->GetValue() == leased->GetValue());

    // bound copies of the lease still allow setting value / disabled.
    auto leasedCopy = leased->GetBoundCopy();

    leasedCopy->SetValue(3);

    REQUIRE(original->GetValue() == 3);
    REQUIRE(original->GetValue() == copy->GetValue());
    REQUIRE(original->GetValue() == leased->GetValue());
    REQUIRE(original->GetValue() == leasedCopy->GetValue());

    leasedCopy->SetDisabled(false);
    leasedCopy->SetDisabled(true);

    REQUIRE(leased->Return());

    original->SetValue(1);

    REQUIRE(original->GetValue() == 1);
    REQUIRE(original->GetValue() == copy->GetValue());
    REQUIRE_FALSE(original->IsDisabled());
}

TEST_CASE("CantLeaseFromLease", "[Bindable][Leasing]") {
    auto original = Bindable<int>::Create(1);
    auto leased = original->BeginLease(false);
    REQUIRE_THROWS_AS(leased->BeginLease(false), std::runtime_error);
}

TEST_CASE("CantLeaseFromBindingChain", "[Bindable][Leasing]") {
    auto original = Bindable<int>::Create(1);
    auto bound1 = original->GetBoundCopy();
    auto bound2 = bound1->GetBoundCopy();

    (void)original->BeginLease(false);
    REQUIRE_THROWS_AS(bound2->BeginLease(false), std::runtime_error);
}

TEST_CASE("ReturnFromBoundCopyOfLeaseFails", "[Bindable][Leasing]") {
    auto original = Bindable<int>::Create(1);
    auto leased = original->BeginLease(true);
    auto copy = leased->GetBoundCopy();

    REQUIRE_THROWS_AS(std::dynamic_pointer_cast<LeasedBindable<int>>(copy)->Return(),
                      std::runtime_error);
}

TEST_CASE("UnbindAllReturnsLease", "[Bindable][Leasing]") {
    auto original = Bindable<int>::Create(1);
    auto leased = original->BeginLease(true);
    leased->UnbindAll();
    REQUIRE_NOTHROW(leased->UnbindAll());
}

TEST_CASE("LeasedBoundToMultiple", "[Bindable][Leasing]") {
    auto original = Bindable<int>::Create(1);
    auto leased = original->BeginLease(true);

    auto another = Bindable<int>::Create();
    leased->BindTo(another);
    another->SetValue(3);
    REQUIRE(another->GetValue() == 3);
    REQUIRE(another->GetValue() == leased->GetValue());

    leased->SetValue(4);
    REQUIRE(original->GetValue() == 4);
    REQUIRE(another->GetValue() == 4);
    REQUIRE(original->GetValue() == leased->GetValue());
}
