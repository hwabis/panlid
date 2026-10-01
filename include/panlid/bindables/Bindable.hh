#pragma once

#include "panlid/bindables/Event.hh"
#include "panlid/bindables/IBindable.hh"
#include "panlid/bindables/ValueChangedEvent.hh"
#include <algorithm>
#include <functional>
#include <memory>
#include <stdexcept>
#include <vector>

namespace panlid::bindables {

template <typename T> class LeasedBindable;

template <typename T>
class Bindable : public std::enable_shared_from_this<Bindable<T>>,
                 public IBindableVal<T>,
                 public IBindable { // todo more interfaces
  public:
    Event<ValueChangedEvent<T>> ValueChanged;
    Event<bool> DisabledChanged;
    Event<ValueChangedEvent<T>> DefaultChanged;

    virtual ~Bindable() = default;

    [[nodiscard]] static std::shared_ptr<Bindable<T>> Create(T defaultValue = T{}) {
        struct Enabler : Bindable<T> {
            explicit Enabler(T defaultValue) : Bindable<T>(std::move(defaultValue)) {}
        };
        return std::make_shared<Enabler>(std::move(defaultValue));
    }

    [[nodiscard]] bool IsDisabled() const override { return disabled_; }

    virtual void SetDisabled(bool newDisabled) {
        throwIfLeased();
        if (disabled_ == newDisabled) {
            return;
        }
        DETAILSetDisabled(newDisabled);
    }

    [[nodiscard]] bool IsDefault() const override { return value_ == defaultValue_; }

    [[nodiscard]] virtual const T& GetValue() const { return value_; }

    virtual void SetValue(const T& newValue) {
        // Intentionally no throwIfLeased() here: if a lease disables exclusive access
        // (by setting Disabled = false), anything can write to Value again.
        if (IsDisabled()) {
            throw std::runtime_error("Cannot set value: bindable is disabled.");
        }
        if (value_ == newValue) {
            return;
        }
        DETAILSetValue(value_, newValue);
    }

    [[nodiscard]] virtual const T& GetDefault() const { return defaultValue_; }

    virtual void SetDefault(const T& newDefault) {
        if (IsDisabled()) {
            throw std::runtime_error("Cannot set default value: bindable is disabled.");
        }
        if (defaultValue_ == newDefault) {
            return;
        }
        DETAILSetDefault(defaultValue_, newDefault);
    }

    virtual void DETAILSetDisabled(bool newDisabled, bool bypassChecks = false,
                                   Bindable<T>* source = nullptr) {
        if (!bypassChecks) {
            throwIfLeased();
        }
        disabled_ = newDisabled;
        TriggerDisabledChange(source != nullptr ? source : this, true, bypassChecks);
    }

    virtual void DETAILSetValue(const T& previousValue, const T& newValue,
                                bool bypassChecks = false, Bindable<T>* source = nullptr) {
        value_ = newValue;
        TriggerValueChange(previousValue, source != nullptr ? source : this, true, bypassChecks);
    }

    virtual void DETAILSetDefault(const T& previousValue, const T& newDefault,
                                  bool bypassChecks = false, Bindable<T>* source = nullptr) {
        defaultValue_ = newDefault;
        TriggerDefaultChange(previousValue, source != nullptr ? source : this, true, bypassChecks);
    }

    virtual void CopyTo(Bindable<T>& them) {
        them.SetValue(GetValue());
        them.SetDefault(GetDefault());
        them.SetDisabled(IsDisabled());
    }

    void BindTo(const std::shared_ptr<Bindable<T>>& them) override {
        if (std::ranges::any_of(Bindings, [&](const std::weak_ptr<Bindable<T>>& w) {
                auto locked = w.lock();
                return locked && locked == them;
            })) {
            throw std::invalid_argument("An already bound bindable cannot be bound again.");
        }

        them->CopyTo(*this);

        addWeakReference(them);
        them->addWeakReference(this->weak_from_this());
    }

    void BindValueChanged(const std::function<void(const ValueChangedEvent<T>&)>& onChange,
                          bool runOnceImmediately = false) {
        ValueChanged += onChange;
        if (runOnceImmediately) {
            onChange(ValueChangedEvent<T>(GetValue(), GetValue()));
        }
    }

    void BindDisabledChanged(const std::function<void(bool)>& onChange,
                             bool runOnceImmediately = false) override {
        DisabledChanged += onChange;
        if (runOnceImmediately) {
            onChange(IsDisabled());
        }
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

    void UnbindBindings() override {
        auto bindingsCopy = Bindings;
        for (const auto& weakBinding : bindingsCopy) {
            if (auto locked = weakBinding.lock()) {
                UnbindFrom(*locked);
            }
        }
    }

    void UnbindAll() override { DETAILUnbindAll(); }

    virtual void DETAILUnbindAll() {
        if (leasedBindable_) {
            leasedBindable_->Return();
        }
        UnbindEvents();
        UnbindBindings();
    }

    virtual void UnbindFrom(Bindable<T>& them) {
        removeWeakReference(them.weak_from_this());
        them.removeWeakReference(this->weak_from_this());
    }

    [[nodiscard]] std::shared_ptr<Bindable<T>> GetUnboundCopy() {
        auto newBindable = CreateInstance();
        CopyTo(*newBindable);
        return newBindable;
    }

    [[nodiscard]] std::shared_ptr<LeasedBindable<T>> BeginLease(bool revertValueOnReturn) {
        if (checkForLease(this)) {
            throw std::runtime_error(
                "Attempted to lease a bindable that is already in a leased state.");
        }
        leasedBindable_ =
            LeasedBindable<T>::DETAILCreate(this->shared_from_this(), revertValueOnReturn);
        return leasedBindable_;
    }

    void DETAILEndLease(LeasedBindable<T>* returnedBindable) {
        if (!leasedBindable_) {
            throw std::runtime_error("Attempted to end a lease without beginning one.");
        }
        if (returnedBindable != leasedBindable_.get()) {
            throw std::runtime_error("Attempted to end a lease but returned a different bindable "
                                     "than the one used to start it.");
        }
        leasedBindable_.reset();
    }

  protected:
    explicit Bindable(T defaultValue = T{}) : value_(defaultValue), defaultValue_(defaultValue) {}

    std::vector<std::weak_ptr<Bindable<T>>> Bindings;

    [[nodiscard]] std::shared_ptr<Bindable<T>> CreateInstance() override { return Create(); }

    void TriggerValueChange(const T& previousValue, Bindable<T>* source,
                            bool propagateToBindings = true, bool bypassChecks = false) {
        T beforePropagation = value_;

        if (propagateToBindings) {
            for (const auto& weakBinding : Bindings) {
                auto locked = weakBinding.lock();
                if (!locked || locked.get() == source) {
                    continue;
                }
                locked->DETAILSetValue(previousValue, value_, bypassChecks, this);
            }
        }

        if (beforePropagation == value_) {
            ValueChanged.Invoke(ValueChangedEvent<T>(previousValue, value_));
        }
    }

    void TriggerDefaultChange(const T& previousValue, Bindable<T>* source,
                              bool propagateToBindings = true, bool bypassChecks = false) {
        T beforePropagation = defaultValue_;

        if (propagateToBindings) {
            for (const auto& weakBinding : Bindings) {
                auto locked = weakBinding.lock();
                if (!locked || locked.get() == source) {
                    continue;
                }
                locked->DETAILSetDefault(previousValue, defaultValue_, bypassChecks, this);
            }
        }

        if (beforePropagation == defaultValue_) {
            DefaultChanged.Invoke(ValueChangedEvent<T>(previousValue, defaultValue_));
        }
    }

    void TriggerDisabledChange(Bindable<T>* source, bool propagateToBindings = true,
                               bool bypassChecks = false) {
        bool beforePropagation = disabled_;

        if (propagateToBindings) {
            for (const auto& weakBinding : Bindings) {
                auto locked = weakBinding.lock();
                if (!locked || locked.get() == source) {
                    continue;
                }
                locked->DETAILSetDisabled(disabled_, bypassChecks, this);
            }
        }

        if (beforePropagation == disabled_) {
            DisabledChanged.Invoke(disabled_);
        }
    }

  private:
    T value_{};
    T defaultValue_{};
    bool disabled_{};
    std::shared_ptr<LeasedBindable<T>> leasedBindable_;

    void addWeakReference(std::weak_ptr<Bindable<T>> weakRef) {
        Bindings.push_back(std::move(weakRef));
    }

    void removeWeakReference(const std::weak_ptr<Bindable<T>>& weakRef) {
        std::erase_if(Bindings, [&](const std::weak_ptr<Bindable<T>>& w) {
            auto a = w.lock();
            auto b = weakRef.lock();
            return a && b && a == b;
        });
    }

    [[nodiscard]] bool checkForLease(Bindable<T>* source) {
        if (leasedBindable_) {
            return true;
        }
        bool found = false;
        for (const auto& weakBinding : Bindings) {
            auto locked = weakBinding.lock();
            if (locked && locked.get() != source) {
                found |= locked->checkForLease(this);
            }
        }
        return found;
    }

    void throwIfLeased() {
        if (leasedBindable_) {
            throw std::runtime_error(
                "Cannot perform this operation on a Bindable that is currently in a leased state.");
        }
    }
};

} // namespace panlid::bindables
