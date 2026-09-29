#pragma once

#include "panlid/bindables/Event.hh"
#include "panlid/bindables/ICanBeDisabled.hh"
#include "panlid/bindables/IHasDefaultValue.hh"
#include "panlid/bindables/IHasDescription.hh"
#include "panlid/bindables/IUnbindable.hh"
#include "panlid/bindables/ValueChangedEvent.hh"
#include <concepts>
#include <functional>
#include <memory>
#include <stdexcept>

namespace panlid::bindables {

class IBindable : public virtual ICanBeDisabled,
                  public virtual IHasDefaultValue,
                  public virtual IUnbindable,
                  public virtual IHasDescription,
                  public std::enable_shared_from_this<IBindable> {
  public:
    ~IBindable() override = default;
    virtual void BindTo(const std::shared_ptr<IBindable>& them) = 0;
    virtual std::shared_ptr<IBindable> GetBoundCopy() = 0;

  protected:
    virtual std::shared_ptr<IBindable> CreateInstance() = 0;
    template <typename T>
        requires std::derived_from<T, IBindable>
    static std::shared_ptr<T> GetBoundCopyImplementation(T& source) {
        auto copyUntyped = source.CreateInstance();
        auto copy = std::dynamic_pointer_cast<T>(copyUntyped);

        if (!copy) {
            throw std::runtime_error("Attempted to create a copy of bindable, but the "
                                     "returned instance type did not match.");
        }

        copy->BindTo(source.shared_from_this());
        return copy;
    }
};

template <typename T> class Bindable;

template <typename T>
class IBindableVal : public virtual ICanBeDisabled,
                     public virtual IHasDefaultValue,
                     public virtual IUnbindable,
                     public virtual IHasDescription {
  public:
    ~IBindableVal() override = default;
    Event<ValueChangedEvent<T>> ValueChanged;

    [[nodiscard]] virtual const T& GetValue() const = 0;
    [[nodiscard]] virtual const T& GetDefault() const = 0;
    virtual void BindTo(const std::shared_ptr<IBindableVal<T>>& them) = 0;
    virtual void BindValueChanged(const std::function<void(const ValueChangedEvent<T>&)>& onChange,
                                  bool runOnceImmediately = false) = 0;
    [[nodiscard]] virtual std::shared_ptr<IBindableVal<T>> GetBoundCopy() = 0;

    virtual std::weak_ptr<Bindable<T>> DETAILGetWeakReference() = 0;
};

} // namespace panlid::bindables
