#pragma once

#include "panlid/logging/Logger.hh"
#include "panlid/logging/LoggingTarget.hh"
#include "panlid/threading/ScheduledDelegate.hh"
#include "panlid/timing/IClock.hh"
#include "panlid/timing/StopwatchClock.hh"
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <deque>
#include <format>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace panlid::threading {

class Scheduler {
  public:
    static constexpr int kLogExcessiveQueueLengthInterval = 1000;

    Scheduler()
        : Scheduler{[threadId = std::this_thread::get_id()] {
                        return std::this_thread::get_id() == threadId;
                    },
                    std::make_shared<timing::StopwatchClock>(true)} {}

    Scheduler(std::function<bool()> isCurrentThread, std::shared_ptr<timing::IClock> clock)
        : isCurrentThread_{std::move(isCurrentThread)}, clock_{std::move(clock)} {}

    [[nodiscard]] bool HasPendingTasks() const { return GetTotalPendingTasks() > 0; }

    [[nodiscard]] int GetTotalTasksRun() const { return totalTasksRun_; }

    [[nodiscard]] int GetTotalPendingTasks() const {
        std::scoped_lock lock{queueLock_};
        return static_cast<int>(runQueue_.size() + timedTasks_.size() + perUpdateTasks_.size());
    }

    [[nodiscard]] bool IsMainThread() const { return !isCurrentThread_ || isCurrentThread_(); }

    void UpdateClock(std::shared_ptr<timing::IClock> newClock) {
        if (!newClock) {
            throw std::invalid_argument("newClock must not be null.");
        }

        std::scoped_lock lock{queueLock_};

        if (newClock == clock_) {
            return;
        }

        if (!clock_) {
            // First valid time: everything scheduled so far starts from here.
            for (const auto& sd : timedTasks_) {
                sd->executionTime_ += newClock->GetCurrentTime();
            }
        }

        clock_ = std::move(newClock);
    }

    int Update() {
        int countToRun = 0;

        {
            std::scoped_lock lock{queueLock_};
            queueTimedTasks();
            queuePerUpdateTasks();
            countToRun = static_cast<int>(runQueue_.size());
        }

        int countRun = 0;

        while (countRun < countToRun) {
            const auto sd = getNextTask();

            if (!sd) {
                break;
            }

            sd->runTaskInternal();

            ++totalTasksRun_;
            ++countRun;
        }

        return countRun;
    }

    void CancelDelayedTasks() {
        std::vector<std::shared_ptr<ScheduledDelegate>> cancelled;

        {
            std::scoped_lock lock{queueLock_};
            cancelled.swap(timedTasks_);
        }

        // Upstream cancels while holding queueLock_, which can deadlock with a running task calling
        // Add.
        for (const auto& task : cancelled) {
            task->Cancel();
        }
    }

    std::shared_ptr<ScheduledDelegate> Add(std::function<void()> task, bool forceScheduled = true) {
        if (!forceScheduled && IsMainThread()) {
            task();
            return nullptr;
        }

        auto del = ScheduledDelegate::Create(std::move(task));
        enqueue(del);
        return del;
    }

    void Add(const std::shared_ptr<ScheduledDelegate>& task) {
        if (task->IsCompleted()) {
            throw std::runtime_error(
                "Can not add a ScheduledDelegate that has been already completed.");
        }

        std::scoped_lock lock{queueLock_};
        insertTimedTask(task);

        if (timedTasks_.size() % kLogExcessiveQueueLengthInterval == 0) {
            logging::Logger::Log(std::format("Scheduler {} has {} timed tasks pending",
                                             static_cast<const void*>(this), timedTasks_.size()),
                                 logging::LoggingTarget::Performance);
        }
    }

    std::shared_ptr<ScheduledDelegate> AddDelayed(std::function<void()> task, double timeUntilRun,
                                                  bool repeat = false) {
        // Locked so currentTime() is not read concurrently with UpdateClock.
        std::scoped_lock lock{queueLock_};

        auto del = ScheduledDelegate::Create(std::move(task), currentTime() + timeUntilRun,
                                             repeat ? timeUntilRun : -1);
        Add(del);
        return del;
    }

  private:
    [[nodiscard]] double currentTime() const { return clock_ ? clock_->GetCurrentTime() : 0.0; }

    void queueTimedTasks() {
        if (timedTasks_.empty()) {
            return;
        }

        const double currentTimeLocal = currentTime();

        for (const auto& sd : timedTasks_) {
            if (sd->GetExecutionTime() > currentTimeLocal) {
                continue;
            }

            tasksToRemove_.push_back(sd);

            if (sd->IsCancelled()) {
                continue;
            }

            if (sd->GetRepeatInterval() == 0) {
                perUpdateTasks_.push_back(sd);
                continue;
            }

            if (sd->GetRepeatInterval() > 0) {
                if (timedTasks_.size() > kLogExcessiveQueueLengthInterval) {
                    throw std::invalid_argument("Too many timed tasks are in the queue!");
                }

                sd->setNextExecution(currentTimeLocal);
                tasksToSchedule_.push_back(sd);
            }

            if (!sd->IsCompleted()) {
                enqueue(sd);
            }
        }

        for (const auto& task : tasksToRemove_) {
            if (const auto it = std::ranges::find(timedTasks_, task); it != timedTasks_.end()) {
                timedTasks_.erase(it);
            }
        }

        tasksToRemove_.clear();

        for (const auto& task : tasksToSchedule_) {
            insertTimedTask(task);
        }

        tasksToSchedule_.clear();
    }

    void queuePerUpdateTasks() {
        std::size_t i = 0;

        while (i < perUpdateTasks_.size()) {
            const auto task = perUpdateTasks_[i];

            task->setNextExecution(std::nullopt);

            if (task->IsCancelled()) {
                perUpdateTasks_.erase(perUpdateTasks_.begin() + static_cast<std::ptrdiff_t>(i));
                continue;
            }

            enqueue(task);
            ++i;
        }
    }

    [[nodiscard]] std::shared_ptr<ScheduledDelegate> getNextTask() {
        std::scoped_lock lock{queueLock_};

        if (runQueue_.empty()) {
            return nullptr;
        }

        auto task = std::move(runQueue_.front());
        runQueue_.pop_front();
        return task;
    }

    void enqueue(const std::shared_ptr<ScheduledDelegate>& task) {
        std::scoped_lock lock{queueLock_};
        runQueue_.push_back(task);

        if (runQueue_.size() % kLogExcessiveQueueLengthInterval == 0) {
            logging::Logger::Log(std::format("Scheduler {} has {} tasks pending",
                                             static_cast<const void*>(this), runQueue_.size()),
                                 logging::LoggingTarget::Performance);
        }
    }

    void insertTimedTask(const std::shared_ptr<ScheduledDelegate>& task) {
        const auto position =
            std::ranges::upper_bound(timedTasks_, task->GetExecutionTime(), {},
                                     [](const auto& sd) { return sd->GetExecutionTime(); });
        timedTasks_.insert(position, task);
    }

    std::deque<std::shared_ptr<ScheduledDelegate>> runQueue_;
    std::vector<std::shared_ptr<ScheduledDelegate>> timedTasks_;
    std::vector<std::shared_ptr<ScheduledDelegate>> perUpdateTasks_;
    std::vector<std::shared_ptr<ScheduledDelegate>> tasksToSchedule_;
    std::vector<std::shared_ptr<ScheduledDelegate>> tasksToRemove_;

    std::function<bool()> isCurrentThread_;
    std::shared_ptr<timing::IClock> clock_;

    // Re-entered by AddDelayed -> Add and queueTimedTasks -> enqueue.
    mutable std::recursive_mutex queueLock_;
    std::atomic<int> totalTasksRun_;
};

} // namespace panlid::threading
