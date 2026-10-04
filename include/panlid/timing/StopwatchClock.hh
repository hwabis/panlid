#pragma once

#include "panlid/timing/IAdjustableClock.hh"
#include <chrono>
#include <ratio>

namespace panlid::timing {

class StopwatchClock : public IAdjustableClock {
  public:
    explicit StopwatchClock(bool start = false) {
        if (start) {
            StopwatchClock::Start();
        }
    }

    [[nodiscard]] double GetCurrentTime() const override {
        return getStopwatchCurrentTime() + seekOffset_;
    }

    [[nodiscard]] double GetRate() const override { return rate_; }

    void SetRate(double value) override {
        if (rate_ == value) {
            return;
        }
        const double now = getStopwatchMilliseconds();
        rateChangeAccumulated_ += (now - rateChangeUsed_) * rate_;
        rateChangeUsed_ = now;
        rate_ = value;
    }

    [[nodiscard]] bool IsRunning() const override { return running_; }

    void Start() override {
        if (running_) {
            return;
        }
        startedAt_ = Clock::now();
        running_ = true;
    }

    void Stop() override {
        if (!running_) {
            return;
        }
        elapsedBeforeLastStart_ += Clock::now() - startedAt_;
        running_ = false;
    }

    void Reset() override {
        resetAccumulatedRate();
        elapsedBeforeLastStart_ = Clock::duration::zero();
        running_ = false;
    }

    void Restart() {
        resetAccumulatedRate();
        elapsedBeforeLastStart_ = Clock::duration::zero();
        startedAt_ = Clock::now();
        running_ = true;
    }

    void ResetSpeedAdjustments() override { SetRate(1); }

    bool Seek(double position) override {
        seekOffset_ = position - getStopwatchCurrentTime();
        return true;
    }

  private:
    using Clock = std::chrono::steady_clock;

    [[nodiscard]] double getStopwatchMilliseconds() const {
        auto elapsed = elapsedBeforeLastStart_;
        if (running_) {
            elapsed += Clock::now() - startedAt_;
        }
        return std::chrono::duration<double, std::milli>(elapsed).count();
    }

    [[nodiscard]] double getStopwatchCurrentTime() const {
        return (getStopwatchMilliseconds() - rateChangeUsed_) * rate_ + rateChangeAccumulated_;
    }

    void resetAccumulatedRate() {
        rateChangeAccumulated_ = 0;
        rateChangeUsed_ = 0;
    }

    bool running_ = false;
    Clock::time_point startedAt_;
    Clock::duration elapsedBeforeLastStart_ = Clock::duration::zero();

    double seekOffset_ = 0;
    double rate_ = 1;
    double rateChangeUsed_ = 0;
    double rateChangeAccumulated_ = 0;
};

} // namespace panlid::timing
