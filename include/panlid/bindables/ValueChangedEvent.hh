#pragma once

namespace panlid::bindables {

template <typename T> struct ValueChangedEvent {
    T OldValue{};
    T NewValue{};
};

} // namespace panlid::bindables
