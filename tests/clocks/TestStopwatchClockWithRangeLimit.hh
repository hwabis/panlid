#pragma once

#include "panlid/timing/StopwatchClock.hh"
#include <algorithm>
#include <limits>

namespace panlid::tests::clocks {

class TestStopwatchClockWithRangeLimit : public timing::StopwatchClock {
  public:
    double MaxTime = std::numeric_limits<double>::infinity();

    TestStopwatchClockWithRangeLimit() : StopwatchClock{true} {}

    [[nodiscard]] static double GetMinTime() { return 0; }

    [[nodiscard]] double GetCurrentTime() const override {
        const double currentTime = StopwatchClock::GetCurrentTime();
        const double clamped = std::clamp(currentTime, GetMinTime(), MaxTime);

        if (clamped == currentTime) {
            return clamped;
        }

        if ((GetRate() > 0 && clamped == MaxTime) || (GetRate() < 0 && clamped == GetMinTime())) {
            const_cast<TestStopwatchClockWithRangeLimit*>(this)->Stop();
        }

        return clamped;
    }

    bool Seek(double position) override {
        const double clamped = std::clamp(position, GetMinTime(), MaxTime);

        if (clamped != position) {
            if (position >= MaxTime) {
                Stop();
            }

            Seek(clamped);
            return false;
        }

        return StopwatchClock::Seek(position);
    }
};

} // namespace panlid::tests::clocks
