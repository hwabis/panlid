#pragma once

#include <utility>

namespace panlid::bindables {

template <typename T> struct ValueChangedEvent {
    T GetOldValue() { return oldValue_; }
    T GetNewValue() { return newValue_; }

    constexpr ValueChangedEvent(T oldValue, T newValue)
        : oldValue_(std::move(oldValue)), newValue_(std::move(newValue)) {}

  private:
    T oldValue_;
    T newValue_;
};

} // namespace panlid::bindables
