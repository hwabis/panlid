#include "panlid/timing/StopwatchClock.hh"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <chrono>
#include <thread>

using panlid::timing::StopwatchClock;

namespace {

constexpr std::chrono::milliseconds kSleepDuration{100};

}

TEST_CASE("ResetTime", "[StopwatchClock]") {
    StopwatchClock stopwatchClock;
    stopwatchClock.Start();

    std::this_thread::sleep_for(kSleepDuration);

    REQUIRE(stopwatchClock.GetCurrentTime() > 0);

    stopwatchClock.Stop();
    stopwatchClock.Reset();

    REQUIRE(stopwatchClock.GetCurrentTime() == 0);
}

TEST_CASE("RateUpResetTime", "[StopwatchClock]") {
    StopwatchClock stopwatchClock;
    stopwatchClock.Start();

    std::this_thread::sleep_for(kSleepDuration);

    stopwatchClock.Stop();
    const double stoppedTime = stopwatchClock.GetCurrentTime();
    REQUIRE(stoppedTime > 0);

    stopwatchClock.SetRate(2.0);
    REQUIRE(stopwatchClock.GetCurrentTime() == stoppedTime);

    stopwatchClock.Reset();

    REQUIRE(stopwatchClock.GetCurrentTime() == 0);
}

TEST_CASE("SeekWhileStopped", "[StopwatchClock]") {
    StopwatchClock stopwatchClock;
    stopwatchClock.Seek(5000);
    REQUIRE(stopwatchClock.GetCurrentTime() == 5000);
}

TEST_CASE("SeekWhenNonZero", "[StopwatchClock]") {
    StopwatchClock stopwatchClock;
    stopwatchClock.Start();

    std::this_thread::sleep_for(kSleepDuration);

    stopwatchClock.Stop();
    const double stoppedTime = stopwatchClock.GetCurrentTime();
    REQUIRE(stoppedTime > 0);

    stopwatchClock.Seek(stoppedTime);

    REQUIRE_THAT(stopwatchClock.GetCurrentTime(), Catch::Matchers::WithinAbs(stoppedTime, 1e-9));
}

TEST_CASE("SeekNegativeAdjustRate", "[StopwatchClock]") {
    StopwatchClock stopwatchClock;

    stopwatchClock.Seek(-5000);
    REQUIRE(stopwatchClock.GetCurrentTime() == -5000);

    stopwatchClock.SetRate(2.0);
    stopwatchClock.Start();

    std::this_thread::sleep_for(kSleepDuration);

    stopwatchClock.Stop();
    const double stoppedTime = stopwatchClock.GetCurrentTime();
    REQUIRE(stoppedTime < 0);

    stopwatchClock.Seek(stoppedTime);

    REQUIRE_THAT(stopwatchClock.GetCurrentTime(), Catch::Matchers::WithinAbs(stoppedTime, 1e-9));
}

TEST_CASE("NegativeRate", "[StopwatchClock]") {
    StopwatchClock stopwatchClock;
    stopwatchClock.SetRate(-2.0);
    stopwatchClock.Start();

    std::this_thread::sleep_for(kSleepDuration);

    stopwatchClock.Stop();

    REQUIRE(stopwatchClock.GetCurrentTime() < 0);
}
