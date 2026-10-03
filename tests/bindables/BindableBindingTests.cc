#include "panlid/bindables/Bindable.hh"
#include "panlid/bindables/IUnbindable.hh"
#include "panlid/bindables/ValueChangedEvent.hh"
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

using namespace panlid::bindables;

namespace {

// Port of BindableBindingTest.TestCustomBindable.
class TestCustomBindable : public Bindable<int> {
  public:
    [[nodiscard]] bool IsBound() const { return isBound_; }

    void BindTo(const std::shared_ptr<Bindable<int>>& them) override {
        Bindable<int>::BindTo(them);
        isBound_ = true;
    }

    void UnbindFrom(IUnbindable& them) override {
        Bindable<int>::UnbindFrom(them);
        isBound_ = false;
    }

  protected:
    [[nodiscard]] std::shared_ptr<Bindable<int>> DETAILCreateInstance() override {
        return std::make_shared<TestCustomBindable>();
    }

  private:
    bool isBound_ = false;
};

} // namespace

TEST_CASE("TestBindToAlreadyBound", "[bindable_binding]") {
    auto bindable1 = Bindable<std::string>::Create("default");
    auto bindable2 = bindable1->GetBoundCopy();

    REQUIRE_THROWS_AS(bindable1->BindTo(bindable2), std::invalid_argument);
}

TEST_CASE("TestPropagation", "[bindable_binding]") {
    auto bindable1 = Bindable<std::string>::Create("default");
    auto bindable2 = bindable1->GetBoundCopy();
    auto bindable3 = bindable2->GetBoundCopy();

    REQUIRE(bindable1->GetValue() == "default");
    REQUIRE(bindable2->GetValue() == bindable1->GetValue());
    REQUIRE(bindable3->GetValue() == bindable1->GetValue());

    bindable1->SetValue("new value");

    REQUIRE(bindable1->GetValue() == "new value");
    REQUIRE(bindable2->GetValue() == bindable1->GetValue());
    REQUIRE(bindable3->GetValue() == bindable1->GetValue());
}

TEST_CASE("TestDisabled", "[bindable_binding]") {
    auto bindable1 = Bindable<std::string>::Create("default");
    auto bindable2 = bindable1->GetBoundCopy();
    auto bindable3 = bindable2->GetBoundCopy();

    bindable1->SetDisabled(true);

    REQUIRE_THROWS_AS(bindable1->SetValue("new value"), std::runtime_error);
    REQUIRE_THROWS_AS(bindable2->SetValue("new value"), std::runtime_error);
    REQUIRE_THROWS_AS(bindable3->SetValue("new value"), std::runtime_error);

    bindable1->SetDisabled(false);
    bindable1->SetValue("new value");

    REQUIRE(bindable1->GetValue() == "new value");
    REQUIRE(bindable2->GetValue() == "new value");
    REQUIRE(bindable3->GetValue() == "new value");

    bindable2->SetValue("new value 2");

    REQUIRE(bindable1->GetValue() == "new value 2");
    REQUIRE(bindable2->GetValue() == "new value 2");
    REQUIRE(bindable3->GetValue() == "new value 2");
}

TEST_CASE("TestDefaultChanged", "[bindable_binding]") {
    auto bindable1 = Bindable<std::string>::Create("default");
    auto bindable2 = bindable1->GetBoundCopy();
    auto bindable3 = bindable2->GetBoundCopy();

    int changed1 = 0;
    int changed2 = 0;
    int changed3 = 0;

    bindable1->DefaultChanged += [&](const ValueChangedEvent<std::string>&) { changed1++; };
    bindable2->DefaultChanged += [&](const ValueChangedEvent<std::string>&) { changed2++; };
    bindable3->DefaultChanged += [&](const ValueChangedEvent<std::string>&) { changed3++; };

    bindable1->SetDefault("new value");

    REQUIRE(changed1 == 1);
    REQUIRE(changed2 == 1);
    REQUIRE(changed3 == 1);

    bindable1->SetDefault("new value 2");

    REQUIRE(changed1 == 2);
    REQUIRE(changed2 == 2);
    REQUIRE(changed3 == 2);

    // should not re-fire, as the value hasn't changed.
    bindable1->SetDefault("new value 2");

    REQUIRE(changed1 == 2);
    REQUIRE(changed2 == 2);
    REQUIRE(changed3 == 2);
}

TEST_CASE("TestDefaultChangedWithUpstreamRejection", "[bindable_binding]") {
    auto bindable1 = Bindable<std::string>::Create("won't change");
    auto bindable2 = bindable1->GetBoundCopy();
    auto* raw2 = bindable2.get();

    int changed1 = 0;
    int changed2 = 0;

    bindable1->DefaultChanged += [&](const ValueChangedEvent<std::string>&) { changed1++; };
    bindable2->DefaultChanged += [&, raw2](const ValueChangedEvent<std::string>&) {
        raw2->SetDefault("won't change");
        changed2++;
    };

    bindable1->SetDefault("new value");

    REQUIRE(bindable1->GetDefault() == "won't change");
    REQUIRE(bindable1->GetDefault() == bindable2->GetDefault());

    // bindable1 should only receive the final change, skipping the intermediary (overridden) one.
    REQUIRE(changed1 == 1);
    REQUIRE(changed2 == 2);
}

TEST_CASE("TestValueChanged", "[bindable_binding]") {
    auto bindable1 = Bindable<std::string>::Create("default");
    auto bindable2 = bindable1->GetBoundCopy();
    auto bindable3 = bindable2->GetBoundCopy();

    int changed1 = 0;
    int changed2 = 0;
    int changed3 = 0;

    bindable1->ValueChanged += [&](const ValueChangedEvent<std::string>&) { changed1++; };
    bindable2->ValueChanged += [&](const ValueChangedEvent<std::string>&) { changed2++; };
    bindable3->ValueChanged += [&](const ValueChangedEvent<std::string>&) { changed3++; };

    bindable1->SetValue("new value");

    REQUIRE(changed1 == 1);
    REQUIRE(changed2 == 1);
    REQUIRE(changed3 == 1);

    bindable1->SetValue("new value 2");

    REQUIRE(changed1 == 2);
    REQUIRE(changed2 == 2);
    REQUIRE(changed3 == 2);

    // should not re-fire, as the value hasn't changed.
    bindable1->SetValue("new value 2");

    REQUIRE(changed1 == 2);
    REQUIRE(changed2 == 2);
    REQUIRE(changed3 == 2);
}

TEST_CASE("TestValueChangedWithUpstreamRejection", "[bindable_binding]") {
    auto bindable1 = Bindable<std::string>::Create("won't change");
    auto bindable2 = bindable1->GetBoundCopy();
    auto* raw2 = bindable2.get();

    int changed1 = 0;
    int changed2 = 0;

    bindable1->ValueChanged += [&](const ValueChangedEvent<std::string>&) { changed1++; };
    bindable2->ValueChanged += [&, raw2](const ValueChangedEvent<std::string>&) {
        raw2->SetValue("won't change");
        changed2++;
    };

    bindable1->SetValue("new value");

    REQUIRE(bindable1->GetValue() == "won't change");
    REQUIRE(bindable1->GetValue() == bindable2->GetValue());

    // bindable1 should only receive the final change, skipping the intermediary (overridden) one.
    REQUIRE(changed1 == 1);
    REQUIRE(changed2 == 2);
}

TEST_CASE("TestDisabledChanged", "[bindable_binding]") {
    auto bindable1 = Bindable<std::string>::Create("default");
    auto bindable2 = bindable1->GetBoundCopy();
    auto bindable3 = bindable2->GetBoundCopy();

    bool disabled1 = false;
    bool disabled2 = false;
    bool disabled3 = false;

    bindable1->DisabledChanged += [&](bool v) { disabled1 = v; };
    bindable2->DisabledChanged += [&](bool v) { disabled2 = v; };
    bindable3->DisabledChanged += [&](bool v) { disabled3 = v; };

    bindable1->SetDisabled(true);

    REQUIRE(disabled1);
    REQUIRE(disabled2);
    REQUIRE(disabled3);

    bindable1->SetDisabled(false);

    REQUIRE_FALSE(disabled1);
    REQUIRE_FALSE(disabled2);
    REQUIRE_FALSE(disabled3);
}

TEST_CASE("TestDisabledChangedWithUpstreamRejection", "[bindable_binding]") {
    auto bindable1 = Bindable<std::string>::Create("won't change");
    auto bindable2 = bindable1->GetBoundCopy();
    auto* raw2 = bindable2.get();

    int changed1 = 0;
    int changed2 = 0;

    bindable1->DisabledChanged += [&](bool) { changed1++; };
    bindable2->DisabledChanged += [&, raw2](bool) {
        raw2->SetDisabled(false);
        changed2++;
    };

    bindable1->SetDisabled(true);

    REQUIRE_FALSE(bindable1->IsDisabled());
    REQUIRE_FALSE(bindable2->IsDisabled());

    // bindable1 should only receive the final change, skipping the intermediary (overridden) one.
    REQUIRE(changed1 == 1);
    REQUIRE(changed2 == 2);
}

TEST_CASE("TestUnbindFrom", "[bindable_binding]") {
    auto bindable1 = Bindable<int>::Create(5);
    auto bindable2 = Bindable<int>::Create();
    bindable2->BindTo(bindable1);

    REQUIRE(bindable1->GetValue() == bindable2->GetValue());

    bindable2->UnbindFrom(*bindable1);
    bindable1->SetValue(10);

    REQUIRE(bindable1->GetValue() != bindable2->GetValue());
}

TEST_CASE("TestEventArgs", "[bindable_binding]") {
    auto bindable1 = Bindable<int>::Create();
    auto bindable2 = Bindable<int>::Create();
    bindable2->BindTo(bindable1);

    std::optional<ValueChangedEvent<int>> event1;
    std::optional<ValueChangedEvent<int>> event2;

    bindable1->BindValueChanged([&](const ValueChangedEvent<int>& e) { event1.emplace(e); });
    bindable2->BindValueChanged([&](const ValueChangedEvent<int>& e) { event2.emplace(e); });

    bindable1->SetValue(1);

    REQUIRE(event1.has_value());
    REQUIRE(event2.has_value());
    REQUIRE(event1->OldValue == 0);
    REQUIRE(event1->NewValue == 1);
    REQUIRE(event2->OldValue == 0);
    REQUIRE(event2->NewValue == 1);

    bindable1->SetValue(2);

    REQUIRE(event1->OldValue == 1);
    REQUIRE(event1->NewValue == 2);
    REQUIRE(event2->OldValue == 1);
    REQUIRE(event2->NewValue == 2);
}

TEST_CASE("TestCustomUnbindFromCalledFromUnbindAll", "[bindable_binding]") {
    auto bindable1 = Bindable<int>::Create();
    auto bindable2 = std::make_shared<TestCustomBindable>();

    bindable2->BindTo(bindable1);
    REQUIRE(bindable2->IsBound());

    bindable2->UnbindAll();
    REQUIRE_FALSE(bindable2->IsBound());
}

// todo bindableint, drawable
