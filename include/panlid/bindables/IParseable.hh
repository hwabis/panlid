#pragma once

#include "panlid/bindables/IBindable.hh"
#include <string_view>

namespace panlid::bindables {

class IParseable {
  public:
    virtual ~IParseable() = default;
    virtual void Parse(std::string_view input) = 0;
    virtual void Parse(const IBindable& input) = 0;
};

} // namespace panlid::bindables
