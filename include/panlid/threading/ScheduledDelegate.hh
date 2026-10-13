#pragma once

#include <atomic>
#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>

namespace panlid::threading {

class Scheduler;

class ScheduledDelegate {
  public:
    // First because C++ needs the type declared before the signatures that use it.
    enum class RunState : std::uint8_t {
        Waiting,   // Waiting to run. Potentially not the first run if on a repeating schedule.
        Running,   // Currently running.
        Complete,  // Running completed for a final time.
        Cancelled, // Task manually cancelled.
    };

    // The earliest clock time at which a Scheduler will run this.
    [[nodiscard]] double GetExecutionTime() const { return executionTime_; }

    // Time in milliseconds between repeats of this task. -1 means no repeats.
    [[nodiscard]] double GetRepeatInterval() const { return repeatInterval_; }

    // For a repeating delegate: true runs it as many times as needed to catch up to the current
    // time (a consistent number of runs over real time, even if the scheduler was suspended);
    // false skips the catch-up runs and always uses a future time.
    bool PerformRepeatCatchUpExecutions = true;

    [[nodiscard]] bool IsCompleted() const { return state_ == RunState::Complete; }
    [[nodiscard]] bool IsCancelled() const { return state_ == RunState::Cancelled; }

    [[nodiscard]] RunState GetState() const { return state_; }

    // Always held through a shared_ptr: the scheduler's queues and the caller (who may Cancel)
    // share it.
    [[nodiscard]] static std::shared_ptr<ScheduledDelegate>
    Create(std::function<void()> task, double executionTime = 0, double repeatInterval = -1) {
        return std::shared_ptr<ScheduledDelegate>(
            new ScheduledDelegate(std::move(task), executionTime, repeatInterval));
    }

    // May block if the task is currently running on another thread.
    // Throws std::logic_error if the delegate was cancelled or already completed.
    void RunTask() {
        const std::lock_guard lock(runLock_);

        if (IsCancelled()) {
            throw std::logic_error("Can not run a ScheduledDelegate that has been cancelled.");
        }

        if (IsCompleted()) {
            throw std::logic_error(
                "Can not run a ScheduledDelegate that has been already completed.");
        }

        runTaskInternal();
    }

    // May block if the task is currently running on another thread.
    void Cancel() {
        const std::lock_guard lock(runLock_);
        state_ = RunState::Cancelled;
    }

  private:
    friend class Scheduler;

    double executionTime_;
    const double repeatInterval_;
    std::atomic<RunState> state_{RunState::Waiting};
    std::function<void()> task_;

    ScheduledDelegate(std::function<void()> task, double executionTime, double repeatInterval)
        : executionTime_(executionTime), repeatInterval_(repeatInterval), task_(std::move(task)) {}

    std::recursive_mutex runLock_;

    // Runs the task without throwing on an incorrect state.
    void runTaskInternal() {
        const std::lock_guard lock(runLock_);

        if (state_ != RunState::Waiting) {
            return;
        }

        state_ = RunState::Running;

        assert(task_);
        task_();

        // The task may have cancelled itself while running.
        if (state_ == RunState::Cancelled) {
            return;
        }

        // osu uses Trace.Assert here (also active in release builds).
        assert(state_ == RunState::Running);
        state_ = RunState::Complete;
    }

    void setNextExecution(std::optional<double> currentTime) {
        const std::lock_guard lock(runLock_);

        if (state_ == RunState::Cancelled) {
            return;
        }

        state_ = RunState::Waiting;

        if (currentTime) {
            executionTime_ += repeatInterval_;

            if (executionTime_ < *currentTime && !PerformRepeatCatchUpExecutions) {
                executionTime_ = *currentTime + repeatInterval_;
            }
        }
    }
};

} // namespace panlid::threading
