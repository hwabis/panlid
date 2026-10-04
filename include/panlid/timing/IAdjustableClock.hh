#pragma once

#include "panlid/timing/IClock.hh"

namespace panlid::timing {

class IAdjustableClock : public virtual IClock {
  public:
    ~IAdjustableClock() override = default;

    virtual void Reset() = 0;
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual bool Seek(double position) = 0;
    virtual void SetRate(double rate) = 0;
    virtual void ResetSpeedAdjustments() = 0;
};

} // namespace panlid::timing
