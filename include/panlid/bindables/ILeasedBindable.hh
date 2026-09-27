#pragma once

#include "panlid/bindables/IBindable.hh"

namespace panlid::bindables {

class ILeasedBindable : public virtual IBindable {
  public:
    ~ILeasedBindable() override = default;
    virtual bool Return() = 0;
};

template <typename T>
class ILeasedBindableVal : public virtual ILeasedBindable, public virtual IBindableVal<T> {
  public:
    ~ILeasedBindableVal() override = default;
};

} // namespace panlid::bindables
