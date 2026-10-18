#pragma once

#include "panlid/timing/IClock.hh"
#include <memory>

namespace panlid::timing {

class ISourceChangeableClock : public virtual IClock {
  public:
    ~ISourceChangeableClock() override = default;

    [[nodiscard]] virtual const std::shared_ptr<IClock>& GetSource() const = 0;

    virtual void ChangeSource(std::shared_ptr<IClock> source) = 0;
};

} // namespace panlid::timing
