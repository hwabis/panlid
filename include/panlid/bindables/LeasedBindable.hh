#pragma once

#include "panlid/bindables/Bindable.hh"
#include <memory>
#include <stdexcept>

namespace panlid::bindables {

template <typename T> class LeasedBindable : public Bindable<T> {
  public:
    [[nodiscard]] static std::shared_ptr<LeasedBindable>
    Create(const std::shared_ptr<Bindable<T>>& source, bool revertValueOnReturn) {
        if (source == nullptr) {
            throw std::invalid_argument("A leased bindable requires a source.");
        }

        std::shared_ptr<LeasedBindable> leased{new LeasedBindable(source, revertValueOnReturn)};
        leased->begin(*source);
        return leased;
    }

    bool Return() {
        if (!hasSource_) {
            throw std::runtime_error("Must Return from original leased source.");
        }

        if (hasBeenReturned_) {
            return false;
        }

        this->UnbindAll();
        return true;
    }

    void SetDisabled(bool value) override {
        ensureValid();

        if (this->IsDisabled() == value) {
            return;
        }

        this->SetDisabledInternal(value, true);
    }

    void SetValue(const T& value) override {
        ensureValid();

        if (this->GetValue() == value) {
            return;
        }

        this->SetValueInternal(this->GetValue(), value, true);
    }

    void SetDefault(const T& value) override {
        ensureValid();

        if (this->GetDefault() == value) {
            return;
        }

        this->SetDefaultInternal(this->GetDefault(), value, true);
    }

  protected:
    [[nodiscard]] std::shared_ptr<Bindable<T>> CreateInstance() const override {
        return std::shared_ptr<LeasedBindable>(new LeasedBindable());
    }

    void UnbindAllInternal() override {
        if (hasSource_ && !hasBeenReturned_) {
            if (revertValueOnReturn_) {
                SetValue(valueBeforeLease_);
            }

            SetDisabled(disabledBeforeLease_);

            if (const auto source = source_.lock()) {
                source->EndLease(this);
            }

            hasBeenReturned_ = true;
        }

        Bindable<T>::UnbindAllInternal();
    }

  private:
    LeasedBindable() = default;

    LeasedBindable(const std::shared_ptr<Bindable<T>>& source, bool revertValueOnReturn)
        : source_{source}, valueBeforeLease_{source->GetValue()},
          disabledBeforeLease_{source->IsDisabled()}, revertValueOnReturn_{revertValueOnReturn},
          hasSource_{true} {}

    void begin(Bindable<T>& source) {
        this->BindTo(source);
        SetDisabled(true);
    }

    void ensureValid() const {
        if (hasSource_ && hasBeenReturned_) {
            throw std::runtime_error(
                "Cannot perform operations on a LeasedBindable that has been returned.");
        }
    }

    std::weak_ptr<Bindable<T>> source_;
    T valueBeforeLease_{};
    bool disabledBeforeLease_ = false;
    bool revertValueOnReturn_ = false;
    bool hasSource_ = false;
    bool hasBeenReturned_ = false;
};

} // namespace panlid::bindables
