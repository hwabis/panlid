#pragma once

#include <atomic>
#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>

namespace panlid::threading {

class Scheduler;

class ScheduledDelegate {
  public:
    enum class RunState : std::uint8_t { Waiting, Running, Complete, Cancelled };

    bool PerformRepeatCatchUpExecutions = true;

    [[nodiscard]] static std::shared_ptr<ScheduledDelegate>
    Create(std::function<void()> task, double executionTime = 0, double repeatInterval = -1) {
        return std::shared_ptr<ScheduledDelegate>{
            new ScheduledDelegate{std::move(task), executionTime, repeatInterval}};
    }

    [[nodiscard]] double GetExecutionTime() const { return executionTime_; }

    [[nodiscard]] double GetRepeatInterval() const { return repeatInterval_; }

    [[nodiscard]] RunState GetState() const { return state_; }

    [[nodiscard]] bool IsCompleted() const { return state_ == RunState::Complete; }

    [[nodiscard]] bool IsCancelled() const { return state_ == RunState::Cancelled; }

    void RunTask() {
        std::scoped_lock lock{runLock_};

        if (IsCancelled()) {
            throw std::runtime_error("Can not run a ScheduledDelegate that has been cancelled.");
        }

        if (IsCompleted()) {
            throw std::runtime_error(
                "Can not run a ScheduledDelegate that has been already completed.");
        }

        runTaskInternal();
    }

    void Cancel() {
        std::scoped_lock lock{runLock_};
        state_ = RunState::Cancelled;
    }

  private:
    friend class Scheduler;

    ScheduledDelegate(std::function<void()> task, double executionTime, double repeatInterval)
        : executionTime_{executionTime}, repeatInterval_{repeatInterval}, task_{std::move(task)} {}

    void runTaskInternal() {
        std::scoped_lock lock{runLock_};

        if (state_ != RunState::Waiting) {
            return;
        }

        state_ = RunState::Running;

        assert(task_);
        task_();

        if (state_ == RunState::Cancelled) {
            return;
        }

        // Trace.Assert upstream
        assert(state_ == RunState::Running);
        state_ = RunState::Complete;
    }

    void setNextExecution(std::optional<double> currentTime) {
        std::scoped_lock lock{runLock_};

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

    double executionTime_;
    double repeatInterval_;
    std::function<void()> task_;

    // Re-entered when a task cancels itself.
    std::recursive_mutex runLock_;
    std::atomic<RunState> state_{RunState::Waiting};
};

} // namespace panlid::threading
