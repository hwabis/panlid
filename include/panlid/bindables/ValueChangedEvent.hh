#pragma once

#include <utility>

namespace panlid::bindables {

template <typename T> struct ValueChangedEvent {
    const T OldValue;
    const T NewValue;
    constexpr ValueChangedEvent(T oldValue, T newValue)
        : OldValue(std::move(oldValue)), NewValue(std::move(newValue)) {}
};

} // namespace panlid::bindables
