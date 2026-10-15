#pragma once

#include "panlid/timing/IAdjustableClock.hh"
#include <chrono>
#include <ratio>

namespace panlid::timing {

class StopwatchClock : public IAdjustableClock {
  public:
    explicit StopwatchClock(bool start = false) {
        if (start) {
            startStopwatch();
        }
    }

    void Start() override { startStopwatch(); }

    void Stop() override {
        if (running_) {
            elapsed_ += Clock::now() - startedAt_;
            running_ = false;
        }
    }

    [[nodiscard]] bool IsRunning() const override { return running_; }

    [[nodiscard]] double GetCurrentTime() const override {
        return stopwatchCurrentTime() + seekOffset_;
    }

    [[nodiscard]] double GetRate() const override { return rate_; }

    void SetRate(double value) override {
        if (rate_ == value) {
            return;
        }

        const double milliseconds = stopwatchMilliseconds();
        rateChangeAccumulated_ += (milliseconds - rateChangeUsed_) * rate_;
        rateChangeUsed_ = milliseconds;

        rate_ = value;
    }

    void Reset() override {
        resetAccumulatedRate();
        elapsed_ = Clock::duration::zero();
        running_ = false;
    }

    void Restart() {
        Reset();
        startStopwatch();
    }

    void ResetSpeedAdjustments() override { SetRate(1.0); }

    bool Seek(double position) override {
        seekOffset_ = position - stopwatchCurrentTime();
        return true;
    }

  private:
    using Clock = std::chrono::steady_clock;

    [[nodiscard]] double stopwatchCurrentTime() const {
        return (stopwatchMilliseconds() - rateChangeUsed_) * rate_ + rateChangeAccumulated_;
    }

    [[nodiscard]] double stopwatchMilliseconds() const {
        Clock::duration elapsed = elapsed_;
        if (running_) {
            elapsed += Clock::now() - startedAt_;
        }

        return std::chrono::duration<double, std::milli>(elapsed).count();
    }

    void startStopwatch() {
        if (!running_) {
            startedAt_ = Clock::now();
            running_ = true;
        }
    }

    void resetAccumulatedRate() {
        rateChangeAccumulated_ = 0;
        rateChangeUsed_ = 0;
    }

    double seekOffset_{};
    double rateChangeUsed_{};
    double rateChangeAccumulated_{};
    double rate_ = 1.0;

    Clock::duration elapsed_{};
    Clock::time_point startedAt_;
    bool running_{};
};

} // namespace panlid::timing
