#include "panlid/bindables/Bindable.hh"
#include "panlid/bindables/LeasedBindable.hh"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <initializer_list>
#include <memory>
#include <optional>
#include <stdexcept>

using namespace panlid::bindables;

namespace {

void ExpectDisabled(bool expected, const Bindable<int>& first, const Bindable<int>& second) {
    REQUIRE(first.IsDisabled() == expected);
    REQUIRE(second.IsDisabled() == expected);
}

void ExpectValue(int expected, std::initializer_list<const Bindable<int>*> bindables) {
    for (const auto* bindable : bindables) {
        REQUIRE(bindable->GetValue() == expected);
    }
}

} // namespace

struct BindableLeasingTest {
    std::shared_ptr<Bindable<int>> Original = Bindable<int>::Create(1);
};

TEST_CASE_METHOD(BindableLeasingTest, "LeaseAndReturn", "[BindableLeasing]") {
    const bool revert = GENERATE(false, true);
    auto leased = Original->BeginLease(revert);

    REQUIRE(Original->GetValue() == leased->GetValue());

    leased->SetValue(2);

    ExpectValue(2, {Original.get(), leased.get()});

    REQUIRE(leased->Return());

    REQUIRE(Original->GetValue() == (revert ? 1 : 2));
}

TEST_CASE_METHOD(BindableLeasingTest, "LeaseReturnedOnUnbindAll", "[BindableLeasing]") {
    const bool revert = GENERATE(false, true);
    auto leased = Original->BeginLease(revert);

    REQUIRE(Original->GetValue() == leased->GetValue());

    leased->SetValue(2);

    ExpectValue(2, {Original.get(), leased.get()});

    Original->UnbindAll();

    REQUIRE(Original->GetValue() == (revert ? 1 : 2));
}

TEST_CASE_METHOD(BindableLeasingTest, "ConsecutiveLeases", "[BindableLeasing]") {
    auto leased1 = Original->BeginLease(false);
    REQUIRE(leased1->Return());
    auto leased2 = Original->BeginLease(false);
    REQUIRE(leased2->Return());
}

TEST_CASE_METHOD(BindableLeasingTest, "ModifyAfterReturnFail", "[BindableLeasing]") {
    auto leased = Original->BeginLease(false);
    REQUIRE(leased->Return());

    REQUIRE_THROWS_AS(leased->SetValue(2), std::runtime_error);
    REQUIRE_THROWS_AS(leased->SetDisabled(true), std::runtime_error);
}

TEST_CASE_METHOD(BindableLeasingTest, "DoubleReturnSilentlyNoops", "[BindableLeasing]") {
    auto leased = Original->BeginLease(false);

    REQUIRE(leased->Return());
    REQUIRE_FALSE(leased->Return());
}

TEST_CASE_METHOD(BindableLeasingTest, "DoubleLeaseFails", "[BindableLeasing]") {
    Original->BeginLease(false);
    REQUIRE_THROWS_AS(Original->BeginLease(false), std::runtime_error);
}

TEST_CASE_METHOD(BindableLeasingTest, "IncorrectEndLease", "[BindableLeasing]") {
    REQUIRE_THROWS_AS(Original->EndLease(nullptr), std::runtime_error);

    Original->BeginLease(true);

    auto other = Bindable<int>::Create();
    auto otherLease = other->BeginLease(true);
    REQUIRE_THROWS_AS(Original->EndLease(otherLease.get()), std::runtime_error);
}

TEST_CASE_METHOD(BindableLeasingTest, "DisabledStateDuringLease", "[BindableLeasing]") {
    REQUIRE_FALSE(Original->IsDisabled());

    auto leased = Original->BeginLease(true);

    ExpectDisabled(true, *Original, *leased);

    REQUIRE_THROWS_AS(Original->SetDisabled(false), std::runtime_error);

    leased->SetDisabled(false);

    ExpectDisabled(false, *Original, *leased);

    Original->SetValue(2);

    REQUIRE_THROWS_AS(Original->SetDisabled(true), std::runtime_error);
    ExpectDisabled(false, *Original, *leased);

    leased->SetDisabled(true);

    ExpectDisabled(true, *Original, *leased);

    REQUIRE(leased->Return());

    REQUIRE_FALSE(Original->IsDisabled());
}

TEST_CASE_METHOD(BindableLeasingTest, "DisabledChangeViaBindings", "[BindableLeasing]") {
    Original->BeginLease(true);

    auto bound = Original->GetBoundCopy();

    REQUIRE_THROWS_AS(bound->SetDisabled(false), std::runtime_error);
    REQUIRE(Original->IsDisabled());
}

TEST_CASE_METHOD(BindableLeasingTest, "DisabledChangeViaBindingToLeased", "[BindableLeasing]") {
    std::optional<bool> changedState;
    Original->DisabledChanged += [&](bool disabled) { changedState = disabled; };

    auto leased = Original->BeginLease(true);

    auto bound = leased->GetBoundCopy();

    bound->SetDisabled(false);

    REQUIRE(changedState == false);
    REQUIRE_FALSE(Original->IsDisabled());
}

TEST_CASE_METHOD(BindableLeasingTest, "ValueChangeViaBindings", "[BindableLeasing]") {
    Original->BeginLease(true);

    auto bound = Original->GetBoundCopy();

    REQUIRE_THROWS_AS(bound->SetValue(2), std::runtime_error);
    REQUIRE(Original->GetValue() == 1);
}

TEST_CASE_METHOD(BindableLeasingTest, "DisabledRevertedAfterLease", "[BindableLeasing]") {
    const bool revert = GENERATE(false, true);
    std::optional<bool> changedState;

    Original->SetDisabled(true);
    Original->DisabledChanged += [&](bool disabled) { changedState = disabled; };

    auto leased = Original->BeginLease(revert);

    REQUIRE(leased->Return());

    REQUIRE(Original->IsDisabled());
    REQUIRE_FALSE(changedState.has_value());
}

TEST_CASE_METHOD(BindableLeasingTest, "LeaseFromBoundBindable", "[BindableLeasing]") {
    auto copy = Original->GetBoundCopy();

    auto leased = copy->BeginLease(true);

    REQUIRE_THROWS_AS(Original->BeginLease(false), std::runtime_error);
    REQUIRE_THROWS_AS(copy->BeginLease(false), std::runtime_error);

    leased->SetValue(2);

    ExpectValue(2, {Original.get(), copy.get(), leased.get()});

    auto leasedCopy = leased->GetBoundCopy();

    leasedCopy->SetValue(3);

    ExpectValue(3, {Original.get(), copy.get(), leased.get(), leasedCopy.get()});

    leasedCopy->SetDisabled(false);
    leasedCopy->SetDisabled(true);

    REQUIRE(leased->Return());

    Original->SetValue(1);

    ExpectValue(1, {Original.get(), copy.get()});
    REQUIRE_FALSE(Original->IsDisabled());
}

TEST_CASE_METHOD(BindableLeasingTest, "CantLeaseFromLease", "[BindableLeasing]") {
    auto leased = Original->BeginLease(false);
    REQUIRE_THROWS_AS(leased->BeginLease(false), std::runtime_error);
}

TEST_CASE_METHOD(BindableLeasingTest, "CantLeaseFromBindingChain", "[BindableLeasing]") {
    auto bound1 = Original->GetBoundCopy();
    auto bound2 = bound1->GetBoundCopy();

    Original->BeginLease(false);
    REQUIRE_THROWS_AS(bound2->BeginLease(false), std::runtime_error);
}

TEST_CASE_METHOD(BindableLeasingTest, "ReturnFromBoundCopyOfLeaseFails", "[BindableLeasing]") {
    auto leased = Original->BeginLease(true);

    auto copy = std::dynamic_pointer_cast<LeasedBindable<int>>(leased->GetBoundCopy());

    REQUIRE(copy != nullptr);
    REQUIRE_THROWS_AS(copy->Return(), std::runtime_error);
}

TEST_CASE_METHOD(BindableLeasingTest, "UnbindAllReturnsLease", "[BindableLeasing]") {
    auto leased = Original->BeginLease(true);
    leased->UnbindAll();
    leased->UnbindAll();
}

TEST_CASE_METHOD(BindableLeasingTest, "LeasedBoundToMultiple", "[BindableLeasing]") {
    auto leased = Original->BeginLease(false);

    auto another = Bindable<int>::Create();
    leased->BindTo(*another);
    another->SetValue(3);

    ExpectValue(3, {another.get(), leased.get()});

    leased->SetValue(4);

    ExpectValue(4, {Original.get(), another.get(), leased.get()});
}
