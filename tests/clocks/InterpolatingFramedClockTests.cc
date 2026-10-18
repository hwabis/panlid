#include "TestClock.hh"
#include "TestNonAdjustableClock.hh"
#include "panlid/timing/InterpolatingFramedClock.hh"
#include "panlid/timing/StopwatchClock.hh"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <chrono>
#include <memory>
#include <thread>

namespace panlid::tests::clocks {

namespace {

using Catch::Matchers::WithinAbs;

void SleepFor(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds{milliseconds});
}

struct InterpolatingFramedClockTest {
    std::shared_ptr<TestClock> Source = std::make_shared<TestClock>();
    timing::InterpolatingFramedClock Interpolating;

    InterpolatingFramedClockTest() { Interpolating.ChangeSource(Source); }

    void SleepForHalfAllowableError() const {
        SleepFor(static_cast<int>(Interpolating.AllowableErrorMilliseconds / 2));
    }

    void RequireNotBackwardsWithinAllowance(double lastValue, double allowance) const {
        REQUIRE(Interpolating.GetCurrentTime() >= lastValue);
        REQUIRE_THAT(Interpolating.GetCurrentTime(), WithinAbs(Source->CurrentTime, allowance));
    }

    void RunNonElapsingPhase() {
        double lastValue = Interpolating.GetCurrentTime();

        for (int i = 0; i < 100; i++) {
            Interpolating.ProcessFrame();
            REQUIRE(Interpolating.GetCurrentTime() >= lastValue);
            REQUIRE(Interpolating.GetCurrentTime() >= Source->CurrentTime);

            SleepForHalfAllowableError();
            lastValue = Interpolating.GetCurrentTime();
        }
    }

    int RunElapsingPhase() {
        int interpolatedCount = 0;
        double lastValue = Interpolating.GetCurrentTime();

        for (int i = 0; i < 100; i++) {
            Source->CurrentTime += Interpolating.AllowableErrorMilliseconds / 2 + 5;
            Interpolating.ProcessFrame();

            RequireNotBackwardsWithinAllowance(lastValue, Interpolating.AllowableErrorMilliseconds);

            if (Interpolating.IsInterpolating()) {
                interpolatedCount++;
            }

            SleepForHalfAllowableError();
            lastValue = Interpolating.GetCurrentTime();
        }

        return interpolatedCount;
    }

    void SkipSourceForwards() {
        Source->CurrentTime += Interpolating.AllowableErrorMilliseconds * 10 * Source->Rate;
        Interpolating.ProcessFrame();

        REQUIRE_FALSE(Interpolating.IsInterpolating());
        REQUIRE(Interpolating.GetCurrentTime() == Source->CurrentTime);
    }
};

} // namespace

TEST_CASE_METHOD(InterpolatingFramedClockTest, "NeverInterpolatesBackwards",
                 "[InterpolatingFramedClock]") {
    REQUIRE(Interpolating.GetCurrentTime() == Source->CurrentTime);
    Source->Start();
    REQUIRE(Interpolating.GetCurrentTime() == Source->CurrentTime);
    Interpolating.ProcessFrame();

    RunNonElapsingPhase();

    REQUIRE(RunElapsingPhase() > 10);
}

TEST_CASE_METHOD(InterpolatingFramedClockTest, "SourceChangeTransfersValueAdjustable",
                 "[InterpolatingFramedClock]") {
    constexpr double kFirstSourceTime = 256000;
    constexpr double kSecondSourceTime = 128000;

    Source->Seek(kFirstSourceTime);

    const auto secondSource = std::make_shared<TestClock>();
    secondSource->CurrentTime = kSecondSourceTime;

    Interpolating.ProcessFrame();
    REQUIRE(Interpolating.GetCurrentTime() == kFirstSourceTime);

    Interpolating.ChangeSource(secondSource);
    Interpolating.ProcessFrame();

    REQUIRE(secondSource->CurrentTime == kSecondSourceTime);
    REQUIRE(Interpolating.GetCurrentTime() == kSecondSourceTime);
}

TEST_CASE_METHOD(InterpolatingFramedClockTest, "SourceChangeTransfersValueNonAdjustable",
                 "[InterpolatingFramedClock]") {
    constexpr double kFirstSourceTime = 256000;
    constexpr double kSecondSourceTime = 128000;

    Source->Seek(kFirstSourceTime);

    const auto secondSource = std::make_shared<TestNonAdjustableClock>();
    secondSource->CurrentTime = kSecondSourceTime;

    Interpolating.ProcessFrame();
    REQUIRE(Interpolating.GetCurrentTime() == kFirstSourceTime);

    Interpolating.ChangeSource(secondSource);
    Interpolating.ProcessFrame();

    REQUIRE(secondSource->CurrentTime == kSecondSourceTime);
    REQUIRE(Interpolating.GetCurrentTime() == kSecondSourceTime);
}

TEST_CASE_METHOD(InterpolatingFramedClockTest, "NeverInterpolatesBackwardsOnInterpolationFail",
                 "[InterpolatingFramedClock]") {
    constexpr int kSleepTime = 20;

    double lastValue = Interpolating.GetCurrentTime();
    Source->Start();
    int interpolatedCount = 0;

    for (int i = 0; i < 200; i++) {
        Source->Rate += i * 10;

        if (i < 100) {
            Source->CurrentTime += kSleepTime * Source->Rate;
        }

        Interpolating.ProcessFrame();

        if (Interpolating.IsInterpolating()) {
            interpolatedCount++;
        }

        RequireNotBackwardsWithinAllowance(lastValue,
                                           Interpolating.AllowableErrorMilliseconds * Source->Rate);

        SleepFor(kSleepTime);
        lastValue = Interpolating.GetCurrentTime();
    }

    REQUIRE(interpolatedCount > 10);
}

TEST_CASE_METHOD(InterpolatingFramedClockTest, "CanSeekForwardsOnInterpolationFail",
                 "[InterpolatingFramedClock]") {
    constexpr int kSleepTime = 20;

    double lastValue = Interpolating.GetCurrentTime();
    Source->Start();
    int interpolatedCount = 0;

    for (int i = 0; i < 200; i++) {
        Source->Rate += i * 10;

        if (i == 100) {
            SkipSourceForwards();
        } else {
            Source->CurrentTime += kSleepTime * Source->Rate;
            Interpolating.ProcessFrame();
        }

        if (Interpolating.IsInterpolating()) {
            interpolatedCount++;
        }

        RequireNotBackwardsWithinAllowance(lastValue,
                                           Interpolating.AllowableErrorMilliseconds * Source->Rate);

        SleepFor(kSleepTime);
        lastValue = Interpolating.GetCurrentTime();
    }

    REQUIRE(interpolatedCount > 10);
}

TEST_CASE_METHOD(InterpolatingFramedClockTest, "CanSeekBackwards", "[InterpolatingFramedClock]") {
    REQUIRE(Interpolating.GetCurrentTime() == Source->CurrentTime);
    Source->Start();

    REQUIRE(Interpolating.GetCurrentTime() == Source->CurrentTime);
    Interpolating.ProcessFrame();

    Source->Seek(10000);
    Interpolating.ProcessFrame();
    REQUIRE(Interpolating.GetCurrentTime() == Source->CurrentTime);

    Source->Seek(0);
    Interpolating.ProcessFrame();
    REQUIRE(Interpolating.GetCurrentTime() == Source->CurrentTime);
}

TEST_CASE_METHOD(InterpolatingFramedClockTest, "InterpolationAfterSourceStoppedThenSeeked",
                 "[InterpolatingFramedClock]") {
    Interpolating.AllowableErrorMilliseconds = 100000;

    Source->Start();

    while (!Interpolating.IsInterpolating()) {
        Source->CurrentTime += 10;
        SleepFor(10);
        Interpolating.ProcessFrame();
    }

    Source->Stop();
    Source->Seek(-10000);

    Interpolating.ProcessFrame();
    REQUIRE_FALSE(Interpolating.IsInterpolating());
    REQUIRE_THAT(Interpolating.GetCurrentTime(), WithinAbs(-10000, 100));
    REQUIRE_THAT(Interpolating.GetElapsedFrameTime(), WithinAbs(-10000, 100));

    Source->Start();
    Interpolating.ProcessFrame();
    REQUIRE_THAT(Interpolating.GetCurrentTime(), WithinAbs(-10000, 100));
    REQUIRE_THAT(Interpolating.GetElapsedFrameTime(), WithinAbs(0, 100));
}

TEST_CASE_METHOD(InterpolatingFramedClockTest, "NoInterpolationDrift",
                 "[InterpolatingFramedClock]") {
    const int updateRate = GENERATE(0, 1, 10, 50);

    const auto stopwatch = std::make_shared<timing::StopwatchClock>();

    Interpolating.ChangeSource(stopwatch);

    Source->Start();
    stopwatch->Start();

    while (Interpolating.GetCurrentTime() <= 1000) {
        Interpolating.ProcessFrame();
        REQUIRE_THAT(Interpolating.GetCurrentTime(), WithinAbs(stopwatch->GetCurrentTime(), 1));

        SleepFor(updateRate);
    }
}

TEST_CASE_METHOD(InterpolatingFramedClockTest, "InterpolationStaysWithinBounds",
                 "[InterpolatingFramedClock]") {
    Source->Start();

    constexpr int kSleepTime = 20;

    for (int i = 0; i < 100; i++) {
        Source->CurrentTime += kSleepTime;
        Interpolating.ProcessFrame();

        Interpolating.ChangeSource(Source);

        REQUIRE_THAT(Interpolating.GetCurrentTime(),
                     WithinAbs(Source->CurrentTime, Interpolating.AllowableErrorMilliseconds));

        SleepFor(kSleepTime);
    }

    Source->Stop();
    Interpolating.ProcessFrame();

    REQUIRE_FALSE(Interpolating.IsRunning());
    REQUIRE_THAT(Source->CurrentTime, WithinAbs(Interpolating.GetCurrentTime(),
                                                Interpolating.AllowableErrorMilliseconds));
}

} // namespace panlid::tests::clocks
