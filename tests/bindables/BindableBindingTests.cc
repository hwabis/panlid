#include "panlid/bindables/Bindable.hh"
#include "panlid/bindables/IBindable.hh"
#include "panlid/bindables/ValueChangedEvent.hh"
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <stdexcept>
#include <string>

using namespace panlid::bindables;

namespace {

class TestCustomBindable : public Bindable<int> {
  public:
    bool IsBound = false;

    [[nodiscard]] static std::shared_ptr<TestCustomBindable> Create() {
        return std::shared_ptr<TestCustomBindable>(new TestCustomBindable());
    }

    void BindTo(IBindable& them) override {
        Bindable<int>::BindTo(them);
        IsBound = true;
    }

    void UnbindFrom(IBindable& them) override {
        Bindable<int>::UnbindFrom(them);
        IsBound = false;
    }

  protected:
    [[nodiscard]] std::shared_ptr<Bindable<int>> CreateInstance() const override {
        return Create();
    }

  private:
    TestCustomBindable() = default;
};

void ExpectAll(int expected, int first, int second, int third) {
    REQUIRE(first == expected);
    REQUIRE(second == expected);
    REQUIRE(third == expected);
}

} // namespace

TEST_CASE("BindToAlreadyBound", "[BindableBinding]") {
    auto bindable1 = Bindable<std::string>::Create("default");
    auto bindable2 = bindable1->GetBoundCopy();

    REQUIRE_THROWS_AS(bindable1->BindTo(*bindable2), std::invalid_argument);
}

TEST_CASE("Propagation", "[BindableBinding]") {
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

TEST_CASE("Disabled", "[BindableBinding]") {
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

TEST_CASE("DefaultChanged", "[BindableBinding]") {
    auto bindable1 = Bindable<std::string>::Create("default");
    auto bindable2 = bindable1->GetBoundCopy();
    auto bindable3 = bindable2->GetBoundCopy();

    int changed1 = 0;
    int changed2 = 0;
    int changed3 = 0;

    bindable1->DefaultChanged += [&](const ValueChangedEvent<std::string>&) { ++changed1; };
    bindable2->DefaultChanged += [&](const ValueChangedEvent<std::string>&) { ++changed2; };
    bindable3->DefaultChanged += [&](const ValueChangedEvent<std::string>&) { ++changed3; };

    bindable1->SetDefault("new value");
    ExpectAll(1, changed1, changed2, changed3);

    bindable1->SetDefault("new value 2");
    ExpectAll(2, changed1, changed2, changed3);

    bindable1->SetDefault("new value 2");
    ExpectAll(2, changed1, changed2, changed3);
}

TEST_CASE("DefaultChangedWithUpstreamRejection", "[BindableBinding]") {
    auto bindable1 = Bindable<std::string>::Create("won't change");
    auto bindable2 = bindable1->GetBoundCopy();

    int changed1 = 0;
    int changed2 = 0;

    bindable1->DefaultChanged += [&](const ValueChangedEvent<std::string>&) { ++changed1; };
    bindable2->DefaultChanged += [&](const ValueChangedEvent<std::string>&) {
        bindable2->SetDefault("won't change");
        ++changed2;
    };

    bindable1->SetDefault("new value");

    REQUIRE(bindable1->GetDefault() == "won't change");
    REQUIRE(bindable2->GetDefault() == bindable1->GetDefault());

    REQUIRE(changed1 == 1);
    REQUIRE(changed2 == 2);
}

TEST_CASE("ValueChanged", "[BindableBinding]") {
    auto bindable1 = Bindable<std::string>::Create("default");
    auto bindable2 = bindable1->GetBoundCopy();
    auto bindable3 = bindable2->GetBoundCopy();

    int changed1 = 0;
    int changed2 = 0;
    int changed3 = 0;

    bindable1->ValueChanged += [&](const ValueChangedEvent<std::string>&) { ++changed1; };
    bindable2->ValueChanged += [&](const ValueChangedEvent<std::string>&) { ++changed2; };
    bindable3->ValueChanged += [&](const ValueChangedEvent<std::string>&) { ++changed3; };

    bindable1->SetValue("new value");
    ExpectAll(1, changed1, changed2, changed3);

    bindable1->SetValue("new value 2");
    ExpectAll(2, changed1, changed2, changed3);

    bindable1->SetValue("new value 2");
    ExpectAll(2, changed1, changed2, changed3);
}

TEST_CASE("ValueChangedWithUpstreamRejection", "[BindableBinding]") {
    auto bindable1 = Bindable<std::string>::Create("won't change");
    auto bindable2 = bindable1->GetBoundCopy();

    int changed1 = 0;
    int changed2 = 0;

    bindable1->ValueChanged += [&](const ValueChangedEvent<std::string>&) { ++changed1; };
    bindable2->ValueChanged += [&](const ValueChangedEvent<std::string>&) {
        bindable2->SetValue("won't change");
        ++changed2;
    };

    bindable1->SetValue("new value");

    REQUIRE(bindable1->GetValue() == "won't change");
    REQUIRE(bindable2->GetValue() == bindable1->GetValue());

    REQUIRE(changed1 == 1);
    REQUIRE(changed2 == 2);
}

TEST_CASE("DisabledChanged", "[BindableBinding]") {
    auto bindable1 = Bindable<std::string>::Create("default");
    auto bindable2 = bindable1->GetBoundCopy();
    auto bindable3 = bindable2->GetBoundCopy();

    bool disabled1 = false;
    bool disabled2 = false;
    bool disabled3 = false;

    bindable1->DisabledChanged += [&](bool value) { disabled1 = value; };
    bindable2->DisabledChanged += [&](bool value) { disabled2 = value; };
    bindable3->DisabledChanged += [&](bool value) { disabled3 = value; };

    bindable1->SetDisabled(true);

    REQUIRE(disabled1);
    REQUIRE(disabled2);
    REQUIRE(disabled3);

    bindable1->SetDisabled(false);

    REQUIRE_FALSE(disabled1);
    REQUIRE_FALSE(disabled2);
    REQUIRE_FALSE(disabled3);
}

TEST_CASE("DisabledChangedWithUpstreamRejection", "[BindableBinding]") {
    auto bindable1 = Bindable<std::string>::Create("won't change");
    auto bindable2 = bindable1->GetBoundCopy();

    int changed1 = 0;
    int changed2 = 0;

    bindable1->DisabledChanged += [&](bool) { ++changed1; };
    bindable2->DisabledChanged += [&](bool) {
        bindable2->SetDisabled(false);
        ++changed2;
    };

    bindable1->SetDisabled(true);

    REQUIRE_FALSE(bindable1->IsDisabled());
    REQUIRE_FALSE(bindable2->IsDisabled());

    REQUIRE(changed1 == 1);
    REQUIRE(changed2 == 2);
}

TEST_CASE("UnbindFrom", "[BindableBinding]") {
    auto bindable1 = Bindable<int>::Create(5);
    auto bindable2 = Bindable<int>::Create();
    bindable2->BindTo(*bindable1);

    REQUIRE(bindable1->GetValue() == bindable2->GetValue());

    bindable2->UnbindFrom(*bindable1);
    bindable1->SetValue(10);

    REQUIRE(bindable1->GetValue() != bindable2->GetValue());
}

TEST_CASE("UnbindEvents", "[BindableBinding]") {
    auto bindable = Bindable<int>::Create();

    bool valueChanged = false;
    bool defaultChanged = false;
    bool disabledChanged = false;

    bindable->ValueChanged += [&](const ValueChangedEvent<int>&) { valueChanged = true; };
    bindable->DefaultChanged += [&](const ValueChangedEvent<int>&) { defaultChanged = true; };
    bindable->DisabledChanged += [&](bool) { disabledChanged = true; };

    bindable->UnbindEvents();

    bindable->SetValue(5);
    bindable->SetDefault(5);
    bindable->SetDisabled(true);

    REQUIRE_FALSE(valueChanged);
    REQUIRE_FALSE(defaultChanged);
    REQUIRE_FALSE(disabledChanged);
}

TEST_CASE("EventArgs", "[BindableBinding]") {
    auto bindable1 = Bindable<int>::Create();
    auto bindable2 = Bindable<int>::Create();

    bindable2->BindTo(*bindable1);

    ValueChangedEvent<int> event1{};
    ValueChangedEvent<int> event2{};

    bindable1->BindValueChanged([&](const ValueChangedEvent<int>& e) { event1 = e; });
    bindable2->BindValueChanged([&](const ValueChangedEvent<int>& e) { event2 = e; });

    bindable1->SetValue(1);

    REQUIRE(event1.OldValue == 0);
    REQUIRE(event1.NewValue == 1);
    REQUIRE(event2.OldValue == 0);
    REQUIRE(event2.NewValue == 1);

    bindable1->SetValue(2);

    REQUIRE(event1.OldValue == 1);
    REQUIRE(event1.NewValue == 2);
    REQUIRE(event2.OldValue == 1);
    REQUIRE(event2.NewValue == 2);
}

TEST_CASE("CustomUnbindFromCalledFromUnbindAll", "[BindableBinding]") {
    auto bindable1 = Bindable<int>::Create();
    auto bindable2 = TestCustomBindable::Create();

    bindable2->BindTo(*bindable1);
    REQUIRE(bindable2->IsBound);

    bindable2->UnbindAll();
    REQUIRE_FALSE(bindable2->IsBound);
}
