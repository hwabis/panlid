#include "TestClock.hh"
#include "TestStopwatchClockWithRangeLimit.hh"
#include "panlid/timing/DecouplingFramedClock.hh"
#include "panlid/timing/IAdjustableClock.hh"
#include "panlid/timing/StopwatchClock.hh"
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <chrono>
#include <limits>
#include <memory>
#include <thread>

namespace panlid::tests::clocks {

namespace {

using Catch::Matchers::WithinAbs;

constexpr double kTolerance = 30;

void SleepFor(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds{milliseconds});
}

class TestClockWithRange : public TestClock {
  public:
    double MaxTime = std::numeric_limits<double>::infinity();

    [[nodiscard]] static double GetMinTime() { return 0; }

    bool Seek(double position) override {
        if (std::clamp(position, GetMinTime(), MaxTime) != position) {
            return false;
        }

        return TestClock::Seek(position);
    }
};

struct DecouplingFramedClockTest {
    std::shared_ptr<timing::IAdjustableClock> Source = std::make_shared<TestClockWithRange>();
    timing::DecouplingFramedClock DecouplingClock;

    DecouplingFramedClockTest() { DecouplingClock.ChangeSource(Source); }

    void UseRangeLimitedSource(double maxTime = std::numeric_limits<double>::infinity()) {
        auto limited = std::make_shared<TestStopwatchClockWithRangeLimit>();
        limited->MaxTime = maxTime;
        Source = limited;
        DecouplingClock.ChangeSource(Source);
    }

    void RequireTimeNearSource() const {
        REQUIRE_THAT(DecouplingClock.GetCurrentTime(),
                     WithinAbs(Source->GetCurrentTime(), kTolerance));
    }

    void TrackSourceUntilItStops() {
        while (Source->IsRunning()) {
            DecouplingClock.ProcessFrame();
            RequireTimeNearSource();
        }
    }

    void TrackSourceForwardsUntil(double time) {
        double lastTime = DecouplingClock.GetCurrentTime();

        while (DecouplingClock.GetCurrentTime() < time) {
            REQUIRE(Source->IsRunning());
            RequireTimeNearSource();
            REQUIRE(DecouplingClock.GetCurrentTime() >= lastTime);
            lastTime = DecouplingClock.GetCurrentTime();

            DecouplingClock.ProcessFrame();
        }
    }

    void RunForwardsDecoupledUntil(double time) {
        double lastTime = DecouplingClock.GetCurrentTime();

        while (DecouplingClock.GetCurrentTime() < time) {
            REQUIRE_FALSE(Source->IsRunning());
            REQUIRE(DecouplingClock.IsRunning());
            REQUIRE(DecouplingClock.GetCurrentTime() >= lastTime);
            lastTime = DecouplingClock.GetCurrentTime();

            DecouplingClock.ProcessFrame();
        }
    }

    void RunBackwardsDecoupledUntil(double time) {
        double lastTime = DecouplingClock.GetCurrentTime();

        while (DecouplingClock.GetCurrentTime() > time) {
            REQUIRE_FALSE(Source->IsRunning());
            REQUIRE(DecouplingClock.GetCurrentTime() <= lastTime);
            lastTime = DecouplingClock.GetCurrentTime();

            DecouplingClock.ProcessFrame();
        }
    }
};

} // namespace

TEST_CASE_METHOD(DecouplingFramedClockTest, "StartFromDecoupling", "[DecouplingFramedClock]") {
    DecouplingClock.AllowDecoupling = GENERATE(true, false);

    REQUIRE_FALSE(Source->IsRunning());
    REQUIRE_FALSE(DecouplingClock.IsRunning());

    DecouplingClock.Start();
    DecouplingClock.ProcessFrame();

    REQUIRE(Source->IsRunning());
    REQUIRE(DecouplingClock.IsRunning());
}

TEST_CASE_METHOD(DecouplingFramedClockTest, "StartFromSource", "[DecouplingFramedClock]") {
    DecouplingClock.AllowDecoupling = GENERATE(true, false);

    REQUIRE_FALSE(Source->IsRunning());
    REQUIRE_FALSE(DecouplingClock.IsRunning());

    Source->Start();
    DecouplingClock.ProcessFrame();

    REQUIRE(Source->IsRunning());
    REQUIRE(DecouplingClock.IsRunning());
}

TEST_CASE_METHOD(DecouplingFramedClockTest, "SeekFromDecouplingWithoutProcessFrame",
                 "[DecouplingFramedClock]") {
    DecouplingClock.AllowDecoupling = true;

    REQUIRE(Source->GetCurrentTime() == 0);
    REQUIRE(DecouplingClock.GetCurrentTime() == 0);

    DecouplingClock.Start();

    DecouplingClock.Seek(1000);
    DecouplingClock.ProcessFrame();
    REQUIRE(Source->IsRunning());

    DecouplingClock.Seek(-1000);
    REQUIRE_FALSE(Source->IsRunning());

    DecouplingClock.Seek(-1);

    SleepFor(500);
    DecouplingClock.ProcessFrame();

    while (DecouplingClock.GetCurrentTime() < 0) {
        DecouplingClock.ProcessFrame();
    }

    REQUIRE(Source->IsRunning());
}

TEST_CASE_METHOD(DecouplingFramedClockTest, "SeekFromDecoupling", "[DecouplingFramedClock]") {
    DecouplingClock.AllowDecoupling = GENERATE(true, false);

    REQUIRE(Source->GetCurrentTime() == 0);
    REQUIRE(DecouplingClock.GetCurrentTime() == 0);

    DecouplingClock.Seek(1000);

    REQUIRE(Source->GetCurrentTime() == 1000);
    REQUIRE(DecouplingClock.GetCurrentTime() == 0);

    DecouplingClock.ProcessFrame();

    REQUIRE(Source->GetCurrentTime() == 1000);
    REQUIRE(DecouplingClock.GetCurrentTime() == 1000);
}

TEST_CASE_METHOD(DecouplingFramedClockTest, "SeekFromSource", "[DecouplingFramedClock]") {
    DecouplingClock.AllowDecoupling = GENERATE(true, false);

    Source->Start();

    REQUIRE(Source->GetCurrentTime() == 0);
    REQUIRE(DecouplingClock.GetCurrentTime() == 0);

    Source->Seek(1000);
    DecouplingClock.ProcessFrame();

    REQUIRE(Source->GetCurrentTime() == 1000);
    REQUIRE(DecouplingClock.GetCurrentTime() == 1000);
}

TEST_CASE_METHOD(DecouplingFramedClockTest, "ChangeSourceUpdatesToNewSourceTime",
                 "[DecouplingFramedClock]") {
    DecouplingClock.AllowDecoupling = GENERATE(true, false);

    constexpr double kFirstSourceTime = 256000;
    constexpr double kSecondSourceTime = 128000;

    Source->Seek(kFirstSourceTime);
    Source->Start();

    DecouplingClock.ProcessFrame();

    const auto secondSource = std::make_shared<TestClock>();
    secondSource->CurrentTime = kSecondSourceTime;

    REQUIRE(DecouplingClock.GetCurrentTime() == kFirstSourceTime);

    DecouplingClock.ChangeSource(secondSource);
    DecouplingClock.ProcessFrame();

    REQUIRE(secondSource->CurrentTime == kSecondSourceTime);
    REQUIRE(DecouplingClock.GetCurrentTime() == kSecondSourceTime);
}

TEST_CASE_METHOD(DecouplingFramedClockTest, "ChangeSourceUpdatesToCorrectSourceState",
                 "[DecouplingFramedClock]") {
    DecouplingClock.AllowDecoupling = GENERATE(true, false);

    Source->Start();
    DecouplingClock.ProcessFrame();
    REQUIRE(DecouplingClock.IsRunning());

    const auto secondSource = std::make_shared<TestClock>();

    DecouplingClock.ChangeSource(secondSource);
    DecouplingClock.ProcessFrame();
    REQUIRE_FALSE(DecouplingClock.IsRunning());

    DecouplingClock.ChangeSource(Source);
    DecouplingClock.ProcessFrame();
    REQUIRE(DecouplingClock.IsRunning());
}

TEST_CASE_METHOD(DecouplingFramedClockTest, "Reset", "[DecouplingFramedClock]") {
    DecouplingClock.AllowDecoupling = GENERATE(true, false);

    Source->Seek(2000);
    Source->Start();

    DecouplingClock.ProcessFrame();

    REQUIRE(DecouplingClock.IsRunning());
    REQUIRE(DecouplingClock.GetCurrentTime() == 2000);

    DecouplingClock.Reset();
    DecouplingClock.ProcessFrame();

    REQUIRE_FALSE(DecouplingClock.IsRunning());
    REQUIRE_FALSE(Source->IsRunning());
    REQUIRE(DecouplingClock.GetCurrentTime() == 0);
    REQUIRE(Source->GetCurrentTime() == 0);
}

TEST_CASE_METHOD(DecouplingFramedClockTest, "SourceStoppedWhileNotDecoupling",
                 "[DecouplingFramedClock]") {
    DecouplingClock.AllowDecoupling = false;
    DecouplingClock.Start();
    DecouplingClock.ProcessFrame();

    REQUIRE(Source->IsRunning());
    REQUIRE(DecouplingClock.IsRunning());

    Source->Stop();
    DecouplingClock.ProcessFrame();

    REQUIRE_FALSE(Source->IsRunning());
    REQUIRE_FALSE(DecouplingClock.IsRunning());
}

TEST_CASE_METHOD(DecouplingFramedClockTest, "SeekNegativeWhileNotDecoupling",
                 "[DecouplingFramedClock]") {
    DecouplingClock.AllowDecoupling = false;

    REQUIRE_FALSE(DecouplingClock.Seek(-1000));

    REQUIRE(Source->GetCurrentTime() == 0);
    REQUIRE(DecouplingClock.GetCurrentTime() == 0);
}

TEST_CASE_METHOD(DecouplingFramedClockTest, "SeekPositiveWhileNotDecoupling",
                 "[DecouplingFramedClock]") {
    DecouplingClock.AllowDecoupling = false;
    REQUIRE(DecouplingClock.Seek(1000));
    DecouplingClock.ProcessFrame();

    REQUIRE(Source->GetCurrentTime() == 1000);
    REQUIRE(DecouplingClock.GetCurrentTime() == 1000);
}

TEST_CASE_METHOD(DecouplingFramedClockTest, "SourceStoppedWhileDecoupling",
                 "[DecouplingFramedClock]") {
    DecouplingClock.AllowDecoupling = true;
    DecouplingClock.Start();
    DecouplingClock.ProcessFrame();

    REQUIRE(Source->IsRunning());
    REQUIRE(DecouplingClock.IsRunning());

    Source->Stop();

    REQUIRE_FALSE(Source->IsRunning());
    REQUIRE(DecouplingClock.IsRunning());
}

TEST_CASE_METHOD(DecouplingFramedClockTest, "SeekNegativeWhileDecoupling",
                 "[DecouplingFramedClock]") {
    DecouplingClock.AllowDecoupling = true;
    REQUIRE(DecouplingClock.Seek(-1000));

    DecouplingClock.ProcessFrame();

    REQUIRE(Source->GetCurrentTime() == 0);
    REQUIRE(DecouplingClock.GetCurrentTime() == -1000);
}

TEST_CASE_METHOD(DecouplingFramedClockTest, "SeekPositiveWhileDecoupling",
                 "[DecouplingFramedClock]") {
    DecouplingClock.AllowDecoupling = true;
    REQUIRE(DecouplingClock.Seek(1000));
    DecouplingClock.ProcessFrame();

    REQUIRE(Source->GetCurrentTime() == 1000);
    REQUIRE(DecouplingClock.GetCurrentTime() == 1000);
}

TEST_CASE_METHOD(DecouplingFramedClockTest, "SeekBeyondLengthWhileDecoupling",
                 "[DecouplingFramedClock]") {
    UseRangeLimitedSource(500);
    DecouplingClock.AllowDecoupling = true;

    REQUIRE(DecouplingClock.Seek(1000));
    DecouplingClock.ProcessFrame();

    REQUIRE(Source->GetCurrentTime() == 500);
    REQUIRE(DecouplingClock.GetCurrentTime() == 1000);
}

TEST_CASE_METHOD(DecouplingFramedClockTest, "SeekFromNegativeToBeyondLengthWhileDecoupling",
                 "[DecouplingFramedClock]") {
    UseRangeLimitedSource(500);
    DecouplingClock.AllowDecoupling = true;

    DecouplingClock.Start();

    REQUIRE(DecouplingClock.Seek(-1000));
    DecouplingClock.ProcessFrame();

    REQUIRE_THAT(Source->GetCurrentTime(), WithinAbs(0, kTolerance));
    REQUIRE_FALSE(Source->IsRunning());
    REQUIRE(DecouplingClock.GetCurrentTime() == -1000);
    REQUIRE(DecouplingClock.IsRunning());

    REQUIRE(DecouplingClock.Seek(1000));
    DecouplingClock.ProcessFrame();

    REQUIRE(Source->GetCurrentTime() == 500);
    REQUIRE_FALSE(Source->IsRunning());
    REQUIRE_THAT(DecouplingClock.GetCurrentTime(), WithinAbs(1000, kTolerance));
    REQUIRE(DecouplingClock.IsRunning());
}

TEST_CASE_METHOD(DecouplingFramedClockTest, "SeekFromSourceWhileDecoupling",
                 "[DecouplingFramedClock]") {
    DecouplingClock.AllowDecoupling = true;

    REQUIRE(Source->GetCurrentTime() == 0);
    REQUIRE(DecouplingClock.GetCurrentTime() == 0);

    Source->Seek(1000);

    REQUIRE(Source->GetCurrentTime() == 1000);
    REQUIRE(DecouplingClock.GetCurrentTime() != 1000);

    DecouplingClock.Seek(1000);
    DecouplingClock.ProcessFrame();

    REQUIRE(Source->GetCurrentTime() == 1000);
    REQUIRE(DecouplingClock.GetCurrentTime() == 1000);
}

TEST_CASE_METHOD(DecouplingFramedClockTest, "StartFromNegativeTimeIncrementsCorrectly",
                 "[DecouplingFramedClock]") {
    const bool seekBeforeStart = GENERATE(false, true);

    SleepFor(500);

    DecouplingClock.AllowDecoupling = true;

    if (seekBeforeStart) {
        DecouplingClock.Seek(-300);
        DecouplingClock.Start();
    } else {
        DecouplingClock.Start();
        DecouplingClock.Seek(-300);
    }

    DecouplingClock.ProcessFrame();

    REQUIRE_FALSE(Source->IsRunning());
    REQUIRE(Source->GetCurrentTime() == 0);

    const double time = DecouplingClock.GetCurrentTime();

    REQUIRE(DecouplingClock.IsRunning());
    REQUIRE(DecouplingClock.GetCurrentTime() < 0);

    SleepFor(100);

    DecouplingClock.ProcessFrame();
    REQUIRE(DecouplingClock.GetCurrentTime() < 0);
    REQUIRE(DecouplingClock.GetCurrentTime() > time);
}

TEST_CASE_METHOD(DecouplingFramedClockTest, "BackwardPlaybackOverZeroBoundary",
                 "[DecouplingFramedClock]") {
    UseRangeLimitedSource();
    DecouplingClock.AllowDecoupling = true;

    DecouplingClock.Seek(300);
    DecouplingClock.SetRate(-1);
    DecouplingClock.Start();

    DecouplingClock.ProcessFrame();

    TrackSourceUntilItStops();

    REQUIRE_FALSE(Source->IsRunning());

    RunBackwardsDecoupledUntil(-300);
}

TEST_CASE_METHOD(DecouplingFramedClockTest, "NoDecoupledDrift", "[DecouplingFramedClock]") {
    const int updateRate = GENERATE(0, 1, 10, 50);

    timing::StopwatchClock stopwatch;

    DecouplingClock.Start();
    stopwatch.Start();

    DecouplingClock.Seek(-100);
    stopwatch.Seek(-100);

    while (DecouplingClock.GetCurrentTime() <= 0) {
        DecouplingClock.ProcessFrame();
        REQUIRE_THAT(DecouplingClock.GetCurrentTime(), WithinAbs(stopwatch.GetCurrentTime(), 1));

        SleepFor(updateRate);
    }
}

TEST_CASE_METHOD(DecouplingFramedClockTest, "ForwardPlaybackOverZeroBoundary",
                 "[DecouplingFramedClock]") {
    UseRangeLimitedSource();
    DecouplingClock.AllowDecoupling = true;

    DecouplingClock.Seek(-300);
    DecouplingClock.Start();

    DecouplingClock.ProcessFrame();

    RunForwardsDecoupledUntil(0);

    REQUIRE_THAT(Source->GetCurrentTime(), WithinAbs(DecouplingClock.GetCurrentTime(), kTolerance));
    REQUIRE(Source->IsRunning());

    DecouplingClock.Stop();
    DecouplingClock.ProcessFrame();
    REQUIRE_FALSE(DecouplingClock.IsRunning());
    REQUIRE_FALSE(Source->IsRunning());

    DecouplingClock.Start();
    DecouplingClock.ProcessFrame();
    REQUIRE(DecouplingClock.IsRunning());
    REQUIRE(Source->IsRunning());
}

TEST_CASE_METHOD(DecouplingFramedClockTest, "ForwardPlaybackOverLengthBoundary",
                 "[DecouplingFramedClock]") {
    UseRangeLimitedSource(10000);
    DecouplingClock.AllowDecoupling = true;

    DecouplingClock.Seek(9800);
    DecouplingClock.Start();

    DecouplingClock.ProcessFrame();

    // The decoupling clock lags the source, so stop tracking one tolerance early
    // to keep both clocks inside the same band before the source hits its limit.
    TrackSourceForwardsUntil(10000 - kTolerance);

    while (Source->IsRunning()) {
        DecouplingClock.ProcessFrame();
    }

    REQUIRE_FALSE(Source->IsRunning());
    REQUIRE(DecouplingClock.GetCurrentTime() < 10100);

    RunForwardsDecoupledUntil(10200);

    REQUIRE_FALSE(Source->IsRunning());
}

TEST_CASE_METHOD(DecouplingFramedClockTest, "PlayDifferentSourceAfterSeekFailure",
                 "[DecouplingFramedClock]") {
    DecouplingClock.AllowDecoupling = true;

    const auto firstSource = std::dynamic_pointer_cast<TestClockWithRange>(Source);
    firstSource->MaxTime = 100;

    DecouplingClock.Seek(1000);

    REQUIRE_FALSE(firstSource->IsRunning());

    const auto secondSource = std::make_shared<TestClockWithRange>();

    DecouplingClock.ChangeSource(secondSource);
    DecouplingClock.Start();

    REQUIRE(secondSource->IsRunning());
}

} // namespace panlid::tests::clocks
