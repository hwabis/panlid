#pragma once

namespace panlid::bindables {

class IHasDefaultValue {
  public:
    virtual ~IHasDefaultValue() = default;
    [[nodiscard]] virtual bool IsDefault() const = 0;
};

} // namespace panlid::bindables
