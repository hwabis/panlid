#pragma once

#include "panlid/bindables/BindableParseHelper.hh"
#include "panlid/bindables/Event.hh"
#include "panlid/bindables/IBindable.hh"
#include "panlid/bindables/ValueChangedEvent.hh"
#include "panlid/lists/LockedWeakList.hh"
#include <format>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace panlid::bindables {

template <typename T> class LeasedBindable;

template <typename T>
class Bindable : public IBindable, public std::enable_shared_from_this<Bindable<T>> {
  public:
    Event<ValueChangedEvent<T>> ValueChanged;
    Event<bool> DisabledChanged;
    Event<ValueChangedEvent<T>> DefaultChanged;

    [[nodiscard]] static std::shared_ptr<Bindable> Create(T defaultValue = T{}) {
        return std::shared_ptr<Bindable>(new Bindable(std::move(defaultValue)));
    }

    Bindable(const Bindable&) = delete;
    Bindable& operator=(const Bindable&) = delete;
    Bindable(Bindable&&) = delete;
    Bindable& operator=(Bindable&&) = delete;
    ~Bindable() override = default;

    [[nodiscard]] bool IsDisabled() const override { return disabled_; }

    virtual void SetDisabled(bool value) {
        throwIfLeased();

        if (disabled_ == value) {
            return;
        }

        SetDisabledInternal(value);
    }

    [[nodiscard]] bool IsDefault() const override { return value_ == defaultValue_; }

    void RevertToDefault() { SetValue(defaultValue_); }

    [[nodiscard]] const T& GetValue() const { return value_; }

    virtual void SetValue(const T& value) {
        if (disabled_) {
            throw std::runtime_error("Can not set value as bindable is disabled.");
        }

        if (value_ == value) {
            return;
        }

        SetValueInternal(value_, value);
    }

    [[nodiscard]] const T& GetDefault() const { return defaultValue_; }

    virtual void SetDefault(const T& value) {
        if (disabled_) {
            throw std::runtime_error("Can not set default value as bindable is disabled.");
        }

        if (defaultValue_ == value) {
            return;
        }

        SetDefaultInternal(defaultValue_, value);
    }

    [[nodiscard]] const std::string& GetDescription() const override { return description_; }

    void SetDescription(std::string value) { description_ = std::move(value); }

    virtual void CopyTo(Bindable& them) const {
        them.SetValue(value_);
        them.SetDefault(defaultValue_);
        them.SetDisabled(disabled_);
    }

    void BindTo(IBindable& them) override {
        Bindable& typedThem = castOrThrow(them, "bind to");

        if (bindings_.Contains(typedThem.weak_from_this())) {
            throw std::invalid_argument("An already bound bindable cannot be bound again.");
        }

        typedThem.CopyTo(*this);

        addWeakReference(typedThem.weak_from_this());
        typedThem.addWeakReference(this->weak_from_this());
    }

    void BindValueChanged(std::function<void(const ValueChangedEvent<T>&)> onChange,
                          bool runOnceImmediately = false) {
        ValueChanged += onChange;

        if (runOnceImmediately) {
            onChange(ValueChangedEvent<T>{.OldValue = value_, .NewValue = value_});
        }
    }

    void BindDisabledChanged(std::function<void(bool)> onChange, bool runOnceImmediately = false) {
        DisabledChanged += onChange;

        if (runOnceImmediately) {
            onChange(disabled_);
        }
    }

    void Parse(std::string_view input) { SetValue(ParseValue<T>(input)); }

    void Parse(const IBindable& input) {
        const auto* typed = dynamic_cast<const Bindable*>(&input);
        if (typed == nullptr) {
            throw std::invalid_argument(std::format("Expected bindable of type {}, got {}.",
                                                    typeid(Bindable).name(), typeid(input).name()));
        }

        SetValue(typed->GetValue());
    }

    virtual void TriggerChange() {
        TriggerValueChange(value_, this, false);
        TriggerDisabledChange(this, false);
    }

    void UnbindEvents() override {
        ValueChanged.Clear();
        DefaultChanged.Clear();
        DisabledChanged.Clear();
    }

    void UnbindBindings() final {
        for (const auto& binding : bindings_.GetSnapshot()) {
            UnbindFrom(*binding);
        }
    }

    void UnbindAll() final { UnbindAllInternal(); }

    void UnbindFrom(IBindable& them) override {
        Bindable& typedThem = castOrThrow(them, "unbind");

        removeWeakReference(typedThem.weak_from_this());
        typedThem.removeWeakReference(this->weak_from_this());
    }

    [[nodiscard]] std::shared_ptr<Bindable> GetUnboundCopy() const {
        auto copy = CreateInstance();
        CopyTo(*copy);
        return copy;
    }

    [[nodiscard]] std::shared_ptr<Bindable> GetBoundCopy() {
        auto copy = CreateInstance();
        const Bindable& copyRef = *copy;

        if (typeid(copyRef) != typeid(*this)) {
            throw std::runtime_error(std::format(
                "Attempted to create a copy of {}, but the returned instance type was {}. "
                "Override CreateInstance() for GetBoundCopy() to function properly.",
                typeid(*this).name(), typeid(copyRef).name()));
        }

        copy->BindTo(*this);
        return copy;
    }

    [[nodiscard]] std::shared_ptr<IBindable> GetBoundCopyErased() override {
        return GetBoundCopy();
    }

    std::shared_ptr<LeasedBindable<T>> BeginLease(bool revertValueOnReturn) {
        if (checkForLease(this)) {
            throw std::runtime_error(
                "Attempted to lease a bindable that is already in a leased state.");
        }

        leasedBindable_ = LeasedBindable<T>::Create(this->shared_from_this(), revertValueOnReturn);
        return leasedBindable_;
    }

    void EndLease(const LeasedBindable<T>* returnedBindable) {
        if (leasedBindable_ == nullptr) {
            throw std::runtime_error("Attempted to end a lease without beginning one.");
        }

        if (returnedBindable != leasedBindable_.get()) {
            throw std::runtime_error("Attempted to end a lease but returned a different bindable "
                                     "to the one used to start the lease.");
        }

        leasedBindable_ = nullptr;
    }

  protected:
    explicit Bindable(T defaultValue = T{})
        : value_{defaultValue}, defaultValue_{std::move(defaultValue)} {}

    [[nodiscard]] virtual std::shared_ptr<Bindable> CreateInstance() const { return Create(); }

    void SetDisabledInternal(bool value, bool bypassChecks = false, Bindable* source = nullptr) {
        if (!bypassChecks) {
            throwIfLeased();
        }

        disabled_ = value;
        TriggerDisabledChange(source != nullptr ? source : this, true, bypassChecks);
    }

    void SetValueInternal(T previousValue, const T& value, bool bypassChecks = false,
                          Bindable* source = nullptr) {
        value_ = value;
        TriggerValueChange(std::move(previousValue), source != nullptr ? source : this, true,
                           bypassChecks);
    }

    void SetDefaultInternal(T previousValue, const T& value, bool bypassChecks = false,
                            Bindable* source = nullptr) {
        defaultValue_ = value;
        TriggerDefaultChange(std::move(previousValue), source != nullptr ? source : this, true,
                             bypassChecks);
    }

    void TriggerValueChange(T previousValue, Bindable* source, bool propagateToBindings = true,
                            bool bypassChecks = false) {
        const T beforePropagation = value_;

        if (propagateToBindings) {
            for (const auto& binding : bindings_.GetSnapshot()) {
                if (binding.get() == source) {
                    continue;
                }

                binding->SetValueInternal(previousValue, value_, bypassChecks, this);
            }
        }

        if (beforePropagation == value_) {
            ValueChanged.Invoke(
                ValueChangedEvent<T>{.OldValue = std::move(previousValue), .NewValue = value_});
        }
    }

    void TriggerDefaultChange(T previousValue, Bindable* source, bool propagateToBindings = true,
                              bool bypassChecks = false) {
        const T beforePropagation = defaultValue_;

        if (propagateToBindings) {
            for (const auto& binding : bindings_.GetSnapshot()) {
                if (binding.get() == source) {
                    continue;
                }

                binding->SetDefaultInternal(previousValue, defaultValue_, bypassChecks, this);
            }
        }

        if (beforePropagation == defaultValue_) {
            DefaultChanged.Invoke(ValueChangedEvent<T>{.OldValue = std::move(previousValue),
                                                       .NewValue = defaultValue_});
        }
    }

    void TriggerDisabledChange(Bindable* source, bool propagateToBindings = true,
                               bool bypassChecks = false) {
        const bool beforePropagation = disabled_;

        if (propagateToBindings) {
            for (const auto& binding : bindings_.GetSnapshot()) {
                if (binding.get() == source) {
                    continue;
                }

                binding->SetDisabledInternal(disabled_, bypassChecks, this);
            }
        }

        if (beforePropagation == disabled_) {
            DisabledChanged.Invoke(disabled_);
        }
    }

    virtual void UnbindAllInternal() {
        if (const auto lease = leasedBindable_) {
            lease->Return();
        }

        UnbindEvents();
        UnbindBindings();
    }

  private:
    [[nodiscard]] Bindable& castOrThrow(IBindable& them, std::string_view action) const {
        auto* typed = dynamic_cast<Bindable*>(&them);
        if (typed == nullptr) {
            throw std::runtime_error(
                std::format("Can't {} a bindable of type {} from a bindable of type {}.", action,
                            typeid(them).name(), typeid(*this).name()));
        }

        return *typed;
    }

    void addWeakReference(const std::weak_ptr<Bindable>& reference) { bindings_.Add(reference); }

    void removeWeakReference(const std::weak_ptr<Bindable>& reference) {
        bindings_.Remove(reference);
    }

    [[nodiscard]] bool checkForLease(const Bindable* source) {
        if (leasedBindable_ != nullptr) {
            return true;
        }

        bool found = false;

        for (const auto& binding : bindings_.GetSnapshot()) {
            if (binding.get() != source) {
                found |= binding->checkForLease(this);
            }
        }

        return found;
    }

    void throwIfLeased() const {
        if (leasedBindable_ != nullptr) {
            throw std::runtime_error(
                "Cannot perform this operation on a Bindable that is currently in a leased state.");
        }
    }

    T value_;
    T defaultValue_;
    bool disabled_ = false;
    std::string description_;
    lists::LockedWeakList<Bindable> bindings_;
    std::shared_ptr<LeasedBindable<T>> leasedBindable_;
};

} // namespace panlid::bindables

#include "panlid/bindables/LeasedBindable.hh"
