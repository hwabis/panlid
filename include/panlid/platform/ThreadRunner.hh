#pragma once

#include "panlid/development/ThreadSafety.hh"
#include "panlid/platform/ExecutionMode.hh"
#include "panlid/threading/GameThread.hh"
#include "panlid/threading/GameThreadState.hh"
#include "panlid/threading/InputThread.hh"
#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <format>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <mutex>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <vector>

namespace panlid::platform {

class ThreadRunner {
  public:
    explicit ThreadRunner(std::shared_ptr<threading::InputThread> mainThread)
        : mainThread_{std::move(mainThread)} {
        AddThread(mainThread_);
    }

    ThreadRunner(const ThreadRunner&) = delete;
    ThreadRunner& operator=(const ThreadRunner&) = delete;
    ThreadRunner(ThreadRunner&&) = delete;
    ThreadRunner& operator=(ThreadRunner&&) = delete;
    virtual ~ThreadRunner() = default;

    [[nodiscard]] std::vector<std::shared_ptr<threading::GameThread>> GetThreads() const {
        std::scoped_lock lock{threadsLock_};
        return threads_;
    }

    void SetMaximumUpdateHz(double value) {
        maximumUpdateHz_ = value;
        updateMainThreadRates();
    }

    void SetMaximumInactiveHz(double value) {
        maximumInactiveHz_ = value;
        updateMainThreadRates();
    }

    void SetExecutionMode(ExecutionMode value) { executionMode_ = value; }

    void AddThread(const std::shared_ptr<threading::GameThread>& thread) {
        std::scoped_lock lock{threadsLock_};

        if (std::ranges::find(threads_, thread) == threads_.end()) {
            threads_.push_back(thread);
        }
    }

    void RemoveThread(const std::shared_ptr<threading::GameThread>& thread) {
        std::scoped_lock lock{threadsLock_};
        std::erase(threads_, thread);
    }

    virtual void RunMainLoop() {
        ensureCorrectExecutionMode();

        assert(activeExecutionMode_.has_value());

        switch (*activeExecutionMode_) {
        case ExecutionMode::SingleThread: {
            std::scoped_lock lock{threadsLock_};

            for (const auto& t : threads_) {
                t->runSingleFrame();
            }

            break;
        }

        case ExecutionMode::MultiThreaded:
            mainThread_->runSingleFrame();
            break;
        }

        development::ThreadSafety::ResetAllForCurrentThread();
    }

    void Start() { ensureCorrectExecutionMode(); }

    void Suspend() {
        std::scoped_lock lock{startStopLock_};

        pauseAllThreads();
        activeExecutionMode_.reset();
    }

    void Stop() const {
        constexpr std::chrono::milliseconds kThreadJoinTimeout{30000};

        const auto threads = GetThreads();

        for (const auto& t : std::views::reverse(threads)) {
            const bool hasNativeThread = t->HasNativeThread();

            t->Exit();

            if (hasNativeThread) {
                if (!t->joinNativeThread(kThreadJoinTimeout)) {
                    throw std::runtime_error(
                        std::format("Thread {} failed to exit in allocated time ({}ms).", t->Name,
                                    kThreadJoinTimeout.count()));
                }
            } else {
                t->waitForState(threading::GameThreadState::Exited);
            }

            assert(t->IsExited());
        }

        development::ThreadSafety::ResetAllForCurrentThread();
    }

    // todo port SetCulture (needs GameThread::CurrentCulture and a C++ culture story)

  private:
    void ensureCorrectExecutionMode() {
        const ExecutionMode executionMode = executionMode_;

        {
            std::scoped_lock lock{startStopLock_};

            if (executionMode == activeExecutionMode_) {
                return;
            }

            activeExecutionMode_ = executionMode;
        }

        pauseAllThreads();

        switch (executionMode) {
        case ExecutionMode::MultiThreaded:
            for (const auto& t : GetThreads()) {
                t->Start();
            }

            break;

        case ExecutionMode::SingleThread:
            for (const auto& t : GetThreads()) {
                t->Initialize(t == mainThread_);
            }

            development::ThreadSafety::ResetAllForCurrentThread();
            break;
        }

        updateMainThreadRates();
    }

    void pauseAllThreads() const {
        const auto threads = GetThreads();

        for (const auto& t : std::views::reverse(threads)) {
            t->pause();
        }
    }

    void updateMainThreadRates() {
        std::scoped_lock lock{startStopLock_};

        if (activeExecutionMode_ == ExecutionMode::SingleThread) {
            mainThread_->SetActiveHz(maximumUpdateHz_);
            mainThread_->SetInactiveHz(maximumInactiveHz_);
        } else {
            mainThread_->SetActiveHz(threading::GameThread::kDefaultActiveHz);
            mainThread_->SetInactiveHz(threading::GameThread::kDefaultInactiveHz);
        }
    }

    std::shared_ptr<threading::InputThread> mainThread_;

    std::vector<std::shared_ptr<threading::GameThread>> threads_;
    mutable std::recursive_mutex threadsLock_;

    double maximumUpdateHz_ = threading::GameThread::kDefaultActiveHz;
    double maximumInactiveHz_ = threading::GameThread::kDefaultInactiveHz;

    std::optional<ExecutionMode> activeExecutionMode_;
    std::atomic<ExecutionMode> executionMode_{ExecutionMode::MultiThreaded};

    std::mutex startStopLock_;
};

} // namespace panlid::platform
