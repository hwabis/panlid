#pragma once

#include "panlid/timing/FrameTimeInfo.hh"
#include "panlid/timing/IClock.hh"

namespace panlid::timing {

class IFrameBasedClock : public virtual IClock {
  public:
    ~IFrameBasedClock() override = default;

    [[nodiscard]] virtual double GetElapsedFrameTime() const = 0;
    [[nodiscard]] virtual double GetFramesPerSecond() const = 0;

    [[nodiscard]] FrameTimeInfo GetTimeInfo() const {
        return {.Elapsed = GetElapsedFrameTime(), .Current = GetCurrentTime()};
    }

    virtual void ProcessFrame() = 0;
};

} // namespace panlid::timing
