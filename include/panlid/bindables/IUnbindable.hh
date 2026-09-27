#pragma once

namespace panlid::bindables {

class IUnbindable {
  public:
    virtual ~IUnbindable() = default;
    virtual void UnbindEvents() = 0;
    virtual void UnbindBindings() = 0;
    virtual void UnbindAll() = 0;
    virtual void UnbindFrom(IUnbindable* them) = 0;
};

} // namespace panlid::bindables
