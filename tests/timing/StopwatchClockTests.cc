#include "panlid/timing/StopwatchClock.hh"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <chrono>
#include <thread>

using Catch::Matchers::WithinAbs;
using namespace panlid::timing;
using namespace std::chrono_literals;

namespace {

constexpr auto kSleepTime = 50ms;

} // namespace

TEST_CASE("ResetTime", "[StopwatchClock]") {
    StopwatchClock stopwatchClock;
    stopwatchClock.Start();

    std::this_thread::sleep_for(kSleepTime);

    REQUIRE(stopwatchClock.GetCurrentTime() > 0);

    stopwatchClock.Stop();
    stopwatchClock.Reset();

    REQUIRE(stopwatchClock.GetCurrentTime() == 0.0);
}

TEST_CASE("RateUpResetTime", "[StopwatchClock]") {
    StopwatchClock stopwatchClock;
    stopwatchClock.Start();

    std::this_thread::sleep_for(kSleepTime);

    stopwatchClock.Stop();
    const double stoppedTime = stopwatchClock.GetCurrentTime();
    REQUIRE(stoppedTime > 0);

    stopwatchClock.SetRate(2.0);
    REQUIRE(stopwatchClock.GetCurrentTime() == stoppedTime);

    stopwatchClock.Reset();

    REQUIRE(stopwatchClock.GetCurrentTime() == 0.0);
}

TEST_CASE("SeekWhileStopped", "[StopwatchClock]") {
    StopwatchClock stopwatchClock;
    stopwatchClock.Seek(5000);
    REQUIRE(stopwatchClock.GetCurrentTime() == 5000.0);
}

TEST_CASE("SeekWhenNonZero", "[StopwatchClock]") {
    StopwatchClock stopwatchClock;
    stopwatchClock.Start();

    std::this_thread::sleep_for(kSleepTime);

    stopwatchClock.Stop();
    const double stoppedTime = stopwatchClock.GetCurrentTime();
    REQUIRE(stoppedTime > 0);

    stopwatchClock.Seek(stoppedTime);

    REQUIRE(stopwatchClock.GetCurrentTime() == stoppedTime);
}

TEST_CASE("SeekNegativeAdjustRate", "[StopwatchClock]") {
    StopwatchClock stopwatchClock;

    stopwatchClock.Seek(-5000);
    REQUIRE(stopwatchClock.GetCurrentTime() == -5000.0);

    stopwatchClock.SetRate(2.0);
    stopwatchClock.Start();

    std::this_thread::sleep_for(kSleepTime);

    stopwatchClock.Stop();
    const double stoppedTime = stopwatchClock.GetCurrentTime();
    REQUIRE(stoppedTime < 0);

    stopwatchClock.Seek(stoppedTime);

    // tolerance: the seek offset is recomputed from floating-point differences.
    REQUIRE_THAT(stopwatchClock.GetCurrentTime(), WithinAbs(stoppedTime, 1e-6));
}

TEST_CASE("NegativeRate", "[StopwatchClock]") {
    StopwatchClock stopwatchClock;
    stopwatchClock.SetRate(-2.0);
    stopwatchClock.Start();

    std::this_thread::sleep_for(kSleepTime);

    stopwatchClock.Stop();

    REQUIRE(stopwatchClock.GetCurrentTime() < 0);
}
