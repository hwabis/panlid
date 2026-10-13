#pragma once

#include "panlid/threading/ScheduledDelegate.hh"
#include "panlid/timing/IClock.hh"
#include "panlid/timing/StopwatchClock.hh"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace panlid::threading {

// Marshals delegates to run from the Scheduler's base thread in a threadsafe manner.
class Scheduler {
  public:
    using DelegatePtr = std::shared_ptr<ScheduledDelegate>;

    static constexpr int kLogExcessiveQueueLengthInterval = 1000;

    // Whether there are any tasks queued to run (including delayed tasks in the future).
    [[nodiscard]] bool HasPendingTasks() const { return GetTotalPendingTasks() > 0; }

    // The total tasks this scheduler instance has run.
    [[nodiscard]] int GetTotalTasksRun() const { return totalTasksRun_; }

    // The total number of delegates tracked by this instance for future execution.
    [[nodiscard]] int GetTotalPendingTasks() const {
        const std::lock_guard lock(queueLock_);
        return static_cast<int>(runQueue_.size() + timedTasks_.size() + perUpdateTasks_.size());
    }

    // The base thread is assumed to be the thread on which the constructor runs.
    Scheduler() : Scheduler(nullptr, std::make_shared<timing::StopwatchClock>(true)) {
        isCurrentThread_ = [constructedThread = std::this_thread::get_id()] {
            return std::this_thread::get_id() == constructedThread;
        };
    }

    // An empty isCurrentThread means "always the base thread". A null clock means time 0 until
    // UpdateClock.
    Scheduler(std::function<bool()> isCurrentThread, std::shared_ptr<timing::IClock> clock)
        : isCurrentThread_(std::move(isCurrentThread)), clock_(std::move(clock)) {}

    void UpdateClock(std::shared_ptr<timing::IClock> newClock) {
        if (!newClock) {
            throw std::invalid_argument("newClock must not be null.");
        }

        // C# checks equality before taking the lock; reading clock_ unlocked is a data race here.
        const std::lock_guard lock(queueLock_);

        if (newClock == clock_) {
            return;
        }

        if (!clock_) {
            // First time we get a valid time: treat it as the reference point everything scheduled
            // so far starts from.
            for (const auto& s : timedTasks_) {
                s->executionTime_ += newClock->GetCurrentTime();
            }
        }

        clock_ = std::move(newClock);
    }

    // Whether we are on the base thread.
    [[nodiscard]] bool IsMainThread() const { return !isCurrentThread_ || isCurrentThread_(); }

    // Runs any pending tasks. Returns the number of tasks that were run.
    int Update() {
        int countToRun = 0;
        {
            const std::lock_guard lock(queueLock_);
            queueTimedTasks();
            queuePerUpdateTasks();
            countToRun = static_cast<int>(runQueue_.size());
        }

        if (countToRun == 0) {
            return 0;
        }

        int countRun = 0;

        while (auto sd = getNextTask()) {
            sd->runTaskInternal();

            ++totalTasksRun_;

            if (++countRun == countToRun) {
                break;
            }
        }

        return countRun;
    }

    // Cancels any pending delayed tasks.
    void CancelDelayedTasks() {
        const std::lock_guard lock(queueLock_);

        for (const auto& t : timedTasks_) {
            t->Cancel();
        }

        timedTasks_.clear();
    }

    // Callers usually ignore the returned handle, so the Add functions are not [[nodiscard]].

    // If scheduled, the task runs on the next Update regardless of the clock time.
    // Returns nullptr if the task ran immediately (forceScheduled false and on the base thread).
    DelegatePtr Add(std::function<void()> task, bool forceScheduled = true) {
        if (!forceScheduled && IsMainThread()) {
            // Already on the base thread, no need to schedule.
            task();
            return nullptr;
        }

        auto del = ScheduledDelegate::Create(std::move(task));

        enqueue(del);

        return del;
    }

    // Adds a delegate to the timed list; it runs once its execution time comes.
    // Throws std::logic_error if the delegate has already completed.
    void Add(DelegatePtr task) {
        if (task->IsCompleted()) {
            throw std::logic_error(
                "Can not add a ScheduledDelegate that has been already completed.");
        }

        const std::lock_guard lock(queueLock_);

        addInPlace(std::move(task));

        if (timedTasks_.size() % kLogExcessiveQueueLengthInterval == 0) {
            // TODO(logger): log "{this} has {count} timed tasks pending" plus the first and last
            // task, LoggingTarget::Performance.
        }
    }

    // Runs the task after timeUntilRun milliseconds from the current clock time.
    DelegatePtr AddDelayed(std::function<void()> task, double timeUntilRun, bool repeat = false) {
        // Locked so currentTime can't be read concurrently with a clock update.
        const std::lock_guard lock(queueLock_);

        auto del = ScheduledDelegate::Create(std::move(task), getCurrentTime() + timeUntilRun,
                                             repeat ? timeUntilRun : -1.0);
        Add(del);
        return del;
    }

  private:
    std::queue<DelegatePtr> runQueue_;
    std::vector<DelegatePtr> timedTasks_;
    std::vector<DelegatePtr> perUpdateTasks_;

    std::function<bool()> isCurrentThread_;

    std::shared_ptr<timing::IClock> clock_;

    [[nodiscard]] double getCurrentTime() const { return clock_ ? clock_->GetCurrentTime() : 0.0; }

    mutable std::recursive_mutex queueLock_;

    int totalTasksRun_ = 0;

    std::vector<DelegatePtr> tasksToSchedule_;
    std::vector<DelegatePtr> tasksToRemove_;

    void queueTimedTasks() {
        if (timedTasks_.empty()) {
            return;
        }

        const double currentTimeLocal = getCurrentTime();

        for (const auto& sd : timedTasks_) {
            if (sd->executionTime_ <= currentTimeLocal) {
                tasksToRemove_.push_back(sd);

                if (sd->IsCancelled()) {
                    continue;
                }

                if (sd->GetRepeatInterval() == 0) {
                    // Every-frame tasks are handled slightly differently to reduce overhead.
                    perUpdateTasks_.push_back(sd);
                    continue;
                }

                if (sd->GetRepeatInterval() > 0) {
                    if (timedTasks_.size() > kLogExcessiveQueueLengthInterval) {
                        throw std::invalid_argument("Too many timed tasks are in the queue!");
                    }

                    // Schedule the next repeat of the task.
                    sd->setNextExecution(currentTimeLocal);
                    tasksToSchedule_.push_back(sd);
                }

                if (!sd->IsCompleted()) {
                    enqueue(sd);
                }
            }
        }

        for (const auto& t : tasksToRemove_) {
            // Removes only the first match, like List.Remove.
            if (const auto it = std::ranges::find(timedTasks_, t); it != timedTasks_.end()) {
                timedTasks_.erase(it);
            }
        }

        tasksToRemove_.clear();

        for (const auto& t : tasksToSchedule_) {
            addInPlace(t);
        }

        tasksToSchedule_.clear();
    }

    void queuePerUpdateTasks() {
        for (std::size_t i = 0; i < perUpdateTasks_.size();) {
            const auto& task = perUpdateTasks_[i];

            task->setNextExecution(std::nullopt);

            if (task->IsCancelled()) {
                perUpdateTasks_.erase(perUpdateTasks_.begin() + static_cast<std::ptrdiff_t>(i));
                continue;
            }

            enqueue(task);
            ++i;
        }
    }

    // Returns nullptr when the run queue is empty.
    [[nodiscard]] DelegatePtr getNextTask() {
        const std::lock_guard lock(queueLock_);

        if (runQueue_.empty()) {
            return nullptr;
        }

        DelegatePtr task = std::move(runQueue_.front());
        runQueue_.pop();
        return task;
    }

    void enqueue(DelegatePtr task) {
        const std::lock_guard lock(queueLock_);

        runQueue_.push(std::move(task));

        if (runQueue_.size() % kLogExcessiveQueueLengthInterval == 0) {
            // TODO(logger): log "{this} has {count} tasks pending", LoggingTarget::Performance.
        }
    }

    // Stands in for C#'s List<T>.AddInPlace extension: sorted insert by execution time,
    // equal times keep insertion order.
    void addInPlace(DelegatePtr task) {
        const auto pos =
            std::ranges::upper_bound(timedTasks_, task->executionTime_, {},
                                     [](const DelegatePtr& d) { return d->executionTime_; });
        timedTasks_.insert(pos, std::move(task));
    }
};

} // namespace panlid::threading
