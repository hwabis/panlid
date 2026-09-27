#pragma once

#include <string>

namespace panlid::bindables {

class IHasDescription {
  public:
    virtual ~IHasDescription() = default;
    [[nodiscard]] virtual std::string GetDescription() const = 0;
};

} // namespace panlid::bindables
