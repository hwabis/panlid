#pragma once

#include "panlid/timing/IAdjustableClock.hh"
#include <stdexcept>

namespace panlid::tests::clocks {

class TestClock : public timing::IAdjustableClock {
  public:
    double CurrentTime = 0;
    double Rate = 1;

    [[nodiscard]] double GetCurrentTime() const override { return CurrentTime; }

    [[nodiscard]] double GetRate() const override { return Rate; }

    [[nodiscard]] bool IsRunning() const override { return isRunning_; }

    void Reset() override {
        CurrentTime = 0;
        isRunning_ = false;
    }

    void Start() override { isRunning_ = true; }

    void Stop() override { isRunning_ = false; }

    bool Seek(double position) override {
        CurrentTime = position;
        return true;
    }

    void SetRate(double rate) override { Rate = rate; }

    void ResetSpeedAdjustments() override { throw std::logic_error("Not implemented"); }

  private:
    bool isRunning_ = false;
};

} // namespace panlid::tests::clocks
