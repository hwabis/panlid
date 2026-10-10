#pragma once

#include <utility>

namespace panlid::bindables {

template <typename T> class ValueChangedEvent {
  public:
    constexpr ValueChangedEvent(T oldValue, T newValue)
        : oldValue_(std::move(oldValue)), newValue_(std::move(newValue)) {}

    [[nodiscard]] constexpr const T& GetOldValue() const { return oldValue_; }
    [[nodiscard]] constexpr const T& GetNewValue() const { return newValue_; }

  private:
    T oldValue_;
    T newValue_;
};

} // namespace panlid::bindables
