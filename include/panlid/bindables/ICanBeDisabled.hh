#pragma once

#include "panlid/bindables/Event.hh"
#include <functional>

namespace panlid::bindables {

class ICanBeDisabled {
  public:
    virtual ~ICanBeDisabled() = default;
    Event<bool> DisabledChanged;
    virtual void BindDisabledChanged(const std::function<void(bool)>& onChange,
                                     bool runOnceImmediately = false) = 0;
    [[nodiscard]] virtual bool GetDisabled() const = 0;
};

} // namespace panlid::bindables
