#pragma once

#include "panlid/bindables/Bindable.hh"
#include <memory>
#include <stdexcept>

namespace panlid::bindables {

template <typename T> class LeasedBindable : public Bindable<T> {
    friend class Bindable<T>;

  public:
    ~LeasedBindable() override = default;

    [[nodiscard]] static std::shared_ptr<LeasedBindable<T>>
    DETAILCreate(const std::shared_ptr<Bindable<T>>& source, bool revertValueOnReturn) {
        struct Enabler : LeasedBindable<T> {
            Enabler(const std::shared_ptr<Bindable<T>>& source, bool revertValueOnReturn)
                : LeasedBindable<T>(source, revertValueOnReturn) {}
        };
        return std::make_shared<Enabler>(source, revertValueOnReturn);
    }

    bool Return() {
        if (hasBeenReturned_) {
            return false;
        }
        if (!source_) {
            throw std::runtime_error("Must Return from original leased source.");
        }
        this->UnbindAll();
        return true;
    }

    void DETAILSetValue(const T& previousValue, const T& newValue, bool /*bypassChecks*/ = false,
                        Bindable<T>* /*source*/ = nullptr) override {
        if (source_) {
            checkValid();
        }
        if (this->GetValue() == newValue) {
            return;
        }
        Bindable<T>::DETAILSetValue(previousValue, newValue, true);
    }

    void DETAILSetDefaultValue(const T& previousValue, const T& newDefault,
                               bool /*bypassChecks*/ = false,
                               Bindable<T>* /*source*/ = nullptr) override {
        if (source_) {
            checkValid();
        }
        if (this->GetDefault() == newDefault) {
            return;
        }
        Bindable<T>::DETAILSetDefaultValue(previousValue, newDefault, true, nullptr);
    }

    void DETAILSetDisabled(bool newDisabled, bool /*bypassChecks*/ = false,
                           Bindable<T>* /*source*/ = nullptr) override {
        if (source_) {
            checkValid();
        }
        if (this->IsDisabled() == newDisabled) {
            return;
        }
        Bindable<T>::DETAILSetDisabled(newDisabled, true, nullptr);
    }

    void DETAILUnbindAll() override {
        if (source_ && !hasBeenReturned_) {
            if (revertValueOnReturn_) {
                this->SetValue(valueBeforeLease_);
            }
            this->SetDisabled(disabledBeforeLease_);

            source_->DETAILEndLease(this);
            hasBeenReturned_ = true;
        }
        Bindable<T>::DETAILUnbindAll();
    }

  protected:
    [[nodiscard]] std::shared_ptr<Bindable<T>> DETAILCreateInstance() override {
        struct Enabler : LeasedBindable<T> {
            Enabler() : LeasedBindable<T>() {}
        };
        return std::make_shared<Enabler>();
    }

  private:
    LeasedBindable(const std::shared_ptr<Bindable<T>>& source, bool revertValueOnReturn)
        : Bindable<T>(source ? source->GetValue() : T{}), source_(source),
          revertValueOnReturn_(revertValueOnReturn) {
        if (!source_) {
            throw std::invalid_argument("LeasedBindable source cannot be null.");
        }

        this->BindTo(source_);

        if (revertValueOnReturn_) {
            valueBeforeLease_ = this->GetValue();
        }
        disabledBeforeLease_ = this->IsDisabled();

        Bindable<T>::DETAILSetDisabled(true, true, nullptr);
    }

    LeasedBindable() = default;

    void checkValid() const {
        if (source_ && hasBeenReturned_) {
            throw std::runtime_error(
                "Cannot perform operations on a LeasedBindable that has been Returned.");
        }
    }

    std::shared_ptr<Bindable<T>> source_;
    T valueBeforeLease_{};
    bool disabledBeforeLease_{};
    bool revertValueOnReturn_{};
    bool hasBeenReturned_{};
};

} // namespace panlid::bindables
