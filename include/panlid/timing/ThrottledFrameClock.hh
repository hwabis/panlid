#pragma once

#include "panlid/timing/FramedClock.hh"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <limits>
#include <ratio>
#include <thread>

namespace panlid::timing {

class ThrottledFrameClock : public FramedClock {
  public:
    double MaximumUpdateHz = 1000.0;
    bool Throttling = true;

    [[nodiscard]] double GetTimeSlept() const { return timeSlept_; }

    void ProcessFrame() override {
        assert(MaximumUpdateHz >= 0);

        FramedClock::ProcessFrame();

        if (Throttling) {
            if (MaximumUpdateHz > 0 && MaximumUpdateHz < std::numeric_limits<double>::max()) {
                throttle();
            } else {
                timeSlept_ = sleepAndUpdateCurrent(0);
            }
        } else {
            timeSlept_ = 0;
        }

        assert(timeSlept_ <= GetElapsedFrameTime());
    }

  private:
    static constexpr double kMinimumAccumulatedSleepError = -1000.0 / 30.0;

    void throttle() {
        const double excessFrameTime = 1000.0 / MaximumUpdateHz - GetElapsedFrameTime();

        timeSlept_ = sleepAndUpdateCurrent(std::max(0.0, excessFrameTime + accumulatedSleepError_));

        accumulatedSleepError_ += excessFrameTime - timeSlept_;
        accumulatedSleepError_ = std::max(kMinimumAccumulatedSleepError, accumulatedSleepError_);
    }

    double sleepAndUpdateCurrent(double milliseconds) {
        if (milliseconds <= 0) {
            return 0;
        }

        const double before = GetCurrentTime();

        // todo native sleep ?
        std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(milliseconds));

        const double now = GetSourceTime();
        SetCurrentTime(now);
        return now - before;
    }

    double timeSlept_ = 0;
    double accumulatedSleepError_ = 0;
};

} // namespace panlid::timing
