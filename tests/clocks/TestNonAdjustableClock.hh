#pragma once

#include "panlid/timing/IClock.hh"

namespace panlid::tests::clocks {

class TestNonAdjustableClock : public timing::IClock {
  public:
    double CurrentTime = 0;
    double Rate = 1;

    [[nodiscard]] double GetCurrentTime() const override { return CurrentTime; }

    [[nodiscard]] double GetRate() const override { return Rate; }

    [[nodiscard]] bool IsRunning() const override { return true; }
};

} // namespace panlid::tests::clocks
