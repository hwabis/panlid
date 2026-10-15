#include "panlid/logging/LogEntry.hh"
#include "panlid/logging/Logger.hh"
#include "panlid/logging/LoggingTarget.hh"
#include "panlid/threading/ScheduledDelegate.hh"
#include "panlid/threading/Scheduler.hh"
#include "panlid/timing/StopwatchClock.hh"
#include <algorithm>
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <filesystem>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

using panlid::logging::LogEntry;
using panlid::logging::Logger;
using panlid::logging::LoggingTarget;
using panlid::threading::ScheduledDelegate;
using panlid::threading::Scheduler;
using panlid::timing::StopwatchClock;
using RunState = ScheduledDelegate::RunState;

namespace {

struct SchedulerTest {
    bool FromMainThread{};
    Scheduler Scheduler{[this] { return FromMainThread; }, std::make_shared<StopwatchClock>(true)};
};

class LogCapture {
  public:
    LogCapture(const std::string& name, Logger::EntryListener listener)
        : directory_{std::filesystem::temp_directory_path() / name} {
        std::filesystem::create_directories(directory_);

        Logger::SetStorage(directory_);
        Logger::SetEnabled(true);
        listenerId_ = Logger::AddListener(std::move(listener));
    }

    LogCapture(const LogCapture&) = delete;
    LogCapture& operator=(const LogCapture&) = delete;

    ~LogCapture() {
        Logger::RemoveListener(listenerId_);
        Logger::SetEnabled(false);
        Logger::Flush();

        std::error_code error;
        std::filesystem::remove_all(directory_, error);
    }

  private:
    std::filesystem::path directory_;
    int listenerId_{};
};

Logger::EntryListener CountPerformanceLogs(int& count) {
    return [&count](const LogEntry& entry) {
        if (entry.Target == LoggingTarget::Performance && entry.Message.contains("tasks pending")) {
            ++count;
        }
    };
}

void AddTasks(Scheduler& scheduler, bool withFlushing, int count,
              const std::function<void()>& addTask) {
    for (int i = 0; i < count; ++i) {
        addTask();

        if (withFlushing) {
            scheduler.Update();
        }
    }
}

void ExpectPerformanceLogs(Scheduler& scheduler, bool withFlushing, const std::string& name,
                           const std::function<void()>& addTask) {
    constexpr int kInterval = Scheduler::kLogExcessiveQueueLengthInterval;

    int matchingLogCount = 0;
    const LogCapture capture{name, CountPerformanceLogs(matchingLogCount)};

    std::array<int, 4> observed{};

    observed[0] = matchingLogCount;

    AddTasks(scheduler, withFlushing, kInterval / 2, addTask);
    observed[1] = matchingLogCount;

    AddTasks(scheduler, withFlushing, kInterval / 2, addTask);
    observed[2] = matchingLogCount;

    AddTasks(scheduler, withFlushing, kInterval, addTask);
    observed[3] = matchingLogCount;

    const std::array<int, 4> expected =
        withFlushing ? std::array{0, 0, 0, 0} : std::array{0, 0, 1, 2};
    REQUIRE(observed == expected);
}

} // namespace

TEST_CASE_METHOD(SchedulerTest, "LogOutputFromManyQueuedTasks", "[Scheduler]") {
    const bool withFlushing = GENERATE(false, true);

    ExpectPerformanceLogs(Scheduler, withFlushing, "LogOutputFromManyQueuedTasks",
                          [&] { Scheduler.Add([] {}); });
}

TEST_CASE_METHOD(SchedulerTest, "LogOutputFromManyQueuedScheduledTasks", "[Scheduler]") {
    const bool withFlushing = GENERATE(false, true);

    ExpectPerformanceLogs(Scheduler, withFlushing, "LogOutputFromManyQueuedScheduledTasks",
                          [&] { Scheduler.AddDelayed([] {}, 0); });
}

TEST_CASE_METHOD(SchedulerTest, "ScheduleOnce", "[Scheduler]") {
    const bool onMainThread = GENERATE(false, true);
    const bool forceScheduled = GENERATE(false, true);
    FromMainThread = onMainThread;

    int invocations = 0;

    const auto del = Scheduler.Add([&] { ++invocations; }, forceScheduled);

    if (onMainThread && !forceScheduled) {
        REQUIRE(invocations == 1);
        REQUIRE(del == nullptr);
    } else {
        REQUIRE(invocations == 0);
        REQUIRE(del != nullptr);
    }

    Scheduler.Update();
    REQUIRE(invocations == 1);

    // Ensures that the delegate runs only once in the scheduled/not on main thread branch
    Scheduler.Update();
    REQUIRE(invocations == 1);
}

TEST_CASE_METHOD(SchedulerTest, "CancelScheduledDelegate", "[Scheduler]") {
    int invocations = 0;

    const auto del = ScheduledDelegate::Create([&] { ++invocations; });
    Scheduler.Add(del);

    del->Cancel();

    Scheduler.Update();
    REQUIRE(invocations == 0);
}

TEST_CASE_METHOD(SchedulerTest, "AddCancelledDelegate", "[Scheduler]") {
    int invocations = 0;

    const auto del = ScheduledDelegate::Create([&] { ++invocations; });
    del->Cancel();

    Scheduler.Add(del);

    Scheduler.Update();
    REQUIRE(invocations == 0);
}

TEST_CASE_METHOD(SchedulerTest, "CustomScheduledDelegateRunsOnce", "[Scheduler]") {
    int invocations = 0;

    Scheduler.Add(ScheduledDelegate::Create([&] { ++invocations; }));

    Scheduler.Update();
    REQUIRE(invocations == 1);

    Scheduler.Update();
    REQUIRE(invocations == 1);
}

TEST_CASE_METHOD(SchedulerTest, "DelayedDelegatesAreScheduledWithCorrectTime", "[Scheduler]") {
    const auto clock = std::make_shared<StopwatchClock>();
    Scheduler.UpdateClock(clock);

    auto del = Scheduler.AddDelayed([] {}, 1000);

    REQUIRE(del->GetExecutionTime() == 1000);

    clock->Seek(500);
    del = Scheduler.AddDelayed([] {}, 1000);

    REQUIRE(del->GetExecutionTime() == 1500);
}

TEST_CASE_METHOD(SchedulerTest, "DelayedDelegateDoesNotRunUntilExpectedTime", "[Scheduler]") {
    const auto clock = std::make_shared<StopwatchClock>();
    Scheduler.UpdateClock(clock);

    int invocations = 0;

    Scheduler.AddDelayed([&] { ++invocations; }, 1000);

    clock->Seek(1000);
    Scheduler.AddDelayed([&] { ++invocations; }, 1000);

    for (int time = 0; time <= 2500; time += 100) {
        clock->Seek(time);
        Scheduler.Update();

        int expectedInvocations = 0;

        if (time >= 1000) {
            ++expectedInvocations;
        }

        if (time >= 2000) {
            ++expectedInvocations;
        }

        REQUIRE(invocations == expectedInvocations);
    }
}

TEST_CASE_METHOD(SchedulerTest, "CancelDelayedDelegate", "[Scheduler]") {
    const auto clock = std::make_shared<StopwatchClock>();
    Scheduler.UpdateClock(clock);

    int invocations = 0;

    const auto del = ScheduledDelegate::Create([&] { ++invocations; }, 1000);
    Scheduler.Add(del);
    del->Cancel();

    clock->Seek(1500);
    Scheduler.Update();
    REQUIRE(invocations == 0);
}

TEST_CASE_METHOD(SchedulerTest, "CancelDelayedDelegateDuringRun", "[Scheduler]") {
    const auto clock = std::make_shared<StopwatchClock>();
    Scheduler.UpdateClock(clock);

    int invocations = 0;

    std::shared_ptr<ScheduledDelegate> del;

    del = ScheduledDelegate::Create(
        [&] {
            ++invocations;

            if (del) {
                del->Cancel();
            }
        },
        1000);
    Scheduler.Add(del);

    REQUIRE(del->GetState() == RunState::Waiting);

    clock->Seek(1500);
    Scheduler.Update();
    REQUIRE(invocations == 1);

    REQUIRE(del->GetState() == RunState::Cancelled);

    clock->Seek(2500);
    Scheduler.Update();
    REQUIRE(invocations == 1);
}

TEST_CASE_METHOD(SchedulerTest, "RepeatingDelayedDelegate", "[Scheduler]") {
    const auto clock = std::make_shared<StopwatchClock>();
    Scheduler.UpdateClock(clock);

    int invocations = 0;

    Scheduler.Add(ScheduledDelegate::Create([&] { ++invocations; }, 500, 500));
    Scheduler.Add(ScheduledDelegate::Create([&] { ++invocations; }, 1000, 500));

    for (int time = 0; time <= 2500; time += 100) {
        clock->Seek(time);
        Scheduler.Update();

        int expectedInvocations = 0;

        if (time >= 500) {
            expectedInvocations += 1 + ((time - 500) / 500);
        }

        if (time >= 1000) {
            expectedInvocations += 1 + ((time - 1000) / 500);
        }

        REQUIRE(invocations == expectedInvocations);
    }
}

TEST_CASE_METHOD(SchedulerTest, "ZeroDelayRepeatingDelegate", "[Scheduler]") {
    const auto clock = std::make_shared<StopwatchClock>();
    Scheduler.UpdateClock(clock);

    int invocations = 0;

    REQUIRE(Scheduler.GetTotalPendingTasks() == 0);

    Scheduler.Add(ScheduledDelegate::Create([&] { ++invocations; }, 500, 0));

    REQUIRE(Scheduler.GetTotalPendingTasks() == 1);

    int expectedInvocations = 0;

    for (int time = 0; time <= 2500; time += 100) {
        clock->Seek(time);
        Scheduler.Update();

        if (time >= 500) {
            ++expectedInvocations;
        }

        REQUIRE(invocations == expectedInvocations);
        REQUIRE(Scheduler.GetTotalPendingTasks() == 1);
    }
}

TEST_CASE_METHOD(SchedulerTest, "RepeatingDelayedDelegateCatchUp", "[Scheduler]") {
    const bool performCatchUp = GENERATE(false, true);

    const auto clock = std::make_shared<StopwatchClock>();
    Scheduler.UpdateClock(clock);

    int invocations = 0;

    const auto del = ScheduledDelegate::Create([&] { ++invocations; }, 500, 500);
    del->PerformRepeatCatchUpExecutions = performCatchUp;
    Scheduler.Add(del);

    for (int time = 0; time <= 10000; time += 2000) {
        clock->Seek(time);

        // allow catch-up to potentially occur.
        for (int i = 0; i < 10; ++i) {
            Scheduler.Update();
        }

        const int expectedInvocations = performCatchUp ? time / 500 : time / 2000;

        REQUIRE(invocations == expectedInvocations);
    }
}

TEST_CASE_METHOD(SchedulerTest, "CancelAfterRepeatReschedule", "[Scheduler]") {
    const auto clock = std::make_shared<StopwatchClock>();
    Scheduler.UpdateClock(clock);

    int invocations = 0;

    const auto del = ScheduledDelegate::Create([&] { ++invocations; }, 500, 500);
    Scheduler.Add(del);

    REQUIRE(del->GetState() == RunState::Waiting);

    clock->Seek(750);
    Scheduler.Update();
    REQUIRE(invocations == 1);

    REQUIRE(del->GetState() == RunState::Complete);

    del->Cancel();

    REQUIRE(del->GetState() == RunState::Cancelled);

    clock->Seek(1250);
    Scheduler.Update();
    REQUIRE(invocations == 1);

    REQUIRE(del->GetState() == RunState::Cancelled);
}

TEST_CASE_METHOD(SchedulerTest, "PerUpdateTask", "[Scheduler]") {
    int invocations = 0;

    Scheduler.AddDelayed([&] { ++invocations; }, 0, true);
    REQUIRE(invocations == 0);

    Scheduler.Update();
    REQUIRE(invocations == 1);

    Scheduler.Update();
    REQUIRE(invocations == 2);
}

TEST_CASE_METHOD(SchedulerTest, "ScheduleFromInsideDelegate", "[Scheduler]") {
    constexpr int kMaxReschedules = 3;

    const bool forceScheduled = GENERATE(false, true);
    FromMainThread = !forceScheduled;

    int reschedules = 0;

    std::function<void()> scheduleTask = [&] {
        Scheduler.Add(
            [&] {
                if (reschedules == kMaxReschedules) {
                    return;
                }

                ++reschedules;
                scheduleTask();
            },
            forceScheduled);
    };

    scheduleTask();

    if (forceScheduled) {
        for (int i = 0; i <= kMaxReschedules; ++i) {
            Scheduler.Update();
            REQUIRE(reschedules == std::min(kMaxReschedules, i + 1));
        }
    } else {
        REQUIRE(reschedules == kMaxReschedules);
    }
}

TEST_CASE_METHOD(SchedulerTest, "InvokeBeforeSchedulerRun", "[Scheduler]") {
    int invocations = 0;

    const auto del = ScheduledDelegate::Create([&] { ++invocations; });

    Scheduler.Add(del);
    REQUIRE(invocations == 0);

    del->RunTask();
    REQUIRE(invocations == 1);

    Scheduler.Update();
    REQUIRE(invocations == 1);
}

TEST_CASE_METHOD(SchedulerTest, "InvokeAfterSchedulerRun", "[Scheduler]") {
    int invocations = 0;

    const auto del = ScheduledDelegate::Create([&] { ++invocations; });

    Scheduler.Add(del);
    REQUIRE(invocations == 0);

    Scheduler.Update();
    REQUIRE(invocations == 1);

    REQUIRE_THROWS_AS(del->RunTask(), std::runtime_error);
    REQUIRE(invocations == 1);
}

TEST_CASE_METHOD(SchedulerTest, "InvokeBeforeScheduleUpdate", "[Scheduler]") {
    int invocations = 0;

    const auto del = ScheduledDelegate::Create([&] { ++invocations; });
    Scheduler.Add(del);
    REQUIRE(invocations == 0);

    del->RunTask();
    REQUIRE(invocations == 1);

    Scheduler.Update();
    REQUIRE(invocations == 1);
}

TEST_CASE_METHOD(SchedulerTest, "RepeatAlreadyCompletedSchedule", "[Scheduler]") {
    int invocations = 0;

    const auto del = ScheduledDelegate::Create([&] { ++invocations; });
    del->RunTask();
    REQUIRE(invocations == 1);

    REQUIRE_THROWS_AS(Scheduler.Add(del), std::runtime_error);

    Scheduler.Update();
    REQUIRE(invocations == 1);
}

// Delegate 1 is added at the start. Delegate 2 is added at the start and cancelled by delegate 1.
// Delegate 3 is added during delegate 1's callback and should not get executed.
TEST_CASE_METHOD(SchedulerTest,
                 "DelegateAddedInCallbackNotExecutedAfterIntermediateCancelledDelegate",
                 "[Scheduler]") {
    int invocations = 0;

    const auto cancelled = ScheduledDelegate::Create([] {});

    Scheduler.Add([&] {
        ++invocations;

        cancelled->Cancel();

        Scheduler.Add([&] { ++invocations; });
    });

    Scheduler.Add(cancelled);

    Scheduler.Update();
    REQUIRE(invocations == 1);
}
