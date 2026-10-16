#pragma once

#include <memory>
#include <string>

namespace panlid::bindables {

class IBindable {
  public:
    virtual ~IBindable() = default;

    [[nodiscard]] virtual bool IsDisabled() const = 0;
    [[nodiscard]] virtual bool IsDefault() const = 0;
    [[nodiscard]] virtual const std::string& GetDescription() const = 0;

    virtual void BindTo(IBindable& them) = 0;
    virtual void UnbindFrom(IBindable& them) = 0;
    virtual void UnbindEvents() = 0;
    virtual void UnbindBindings() = 0;
    virtual void UnbindAll() = 0;
    [[nodiscard]] virtual std::shared_ptr<IBindable> GetBoundCopyErased() = 0;
};

} // namespace panlid::bindables
