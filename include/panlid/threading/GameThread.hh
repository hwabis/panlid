#pragma once

#include "panlid/bindables/Bindable.hh"
#include "panlid/bindables/Event.hh"
#include "panlid/bindables/ValueChangedEvent.hh"
#include "panlid/development/ThreadSafety.hh"
#include "panlid/threading/GameThreadState.hh"
#include "panlid/threading/Scheduler.hh"
#include "panlid/timing/ThrottledFrameClock.hh"
#include <atomic>
#include <cassert>
#include <chrono>
#include <exception>
#include <format>
#include <functional>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

namespace panlid::platform {

class ThreadRunner;

}

namespace panlid::threading {

class GameThread {
  public:
    static constexpr int kDefaultActiveHz = 1000;
    static constexpr int kDefaultInactiveHz = 60;

    const std::string Name;
    const std::shared_ptr<bindables::Bindable<bool>> IsActive =
        bindables::Bindable<bool>::Create(true);
    std::function<void(const std::exception_ptr&)> UnhandledException;

    // todo port PerformanceMonitor (monitorPerformance, Monitor, StatisticsCounters, FrameIndex)
    explicit GameThread(std::function<void()> onNewFrame = nullptr, std::string name = "unknown")
        : Name{std::move(name)}, OnNewFrame{std::move(onNewFrame)},
          clock_{std::make_shared<timing::ThrottledFrameClock>()},
          scheduler_{std::make_shared<Scheduler>([this] { return IsCurrent(); }, clock_)} {
        IsActive->BindValueChanged(
            [this](const bindables::ValueChangedEvent<bool>&) { updateMaximumHz(); }, true);
    }

    GameThread(const GameThread&) = delete;
    GameThread& operator=(const GameThread&) = delete;
    GameThread(GameThread&&) = delete;
    GameThread& operator=(GameThread&&) = delete;
    virtual ~GameThread() = default;

    // port when a consumer of GameThread.State appears
    [[nodiscard]] GameThreadState GetState() const { return state_; }

    [[nodiscard]] bool IsRunning() const { return state_ == GameThreadState::Running; }

    [[nodiscard]] bool IsExited() const { return state_ == GameThreadState::Exited; }

    [[nodiscard]] virtual bool IsCurrent() const { return true; }

    [[nodiscard]] const std::shared_ptr<timing::ThrottledFrameClock>& GetClock() const {
        return clock_;
    }

    [[nodiscard]] bool HasNativeThread() const { return hasThread_; }

    [[nodiscard]] const std::shared_ptr<Scheduler>& GetScheduler() const { return scheduler_; }

    // todo port SynchronizationContext (GameThreadSynchronizationContext)
    // todo port CurrentCulture (and ThreadRunner::SetCulture)

    [[nodiscard]] double GetActiveHz() const { return activeHz_; }

    void SetActiveHz(double value) {
        activeHz_ = value;
        updateMaximumHz();
    }

    [[nodiscard]] double GetInactiveHz() const { return inactiveHz_; }

    void SetInactiveHz(double value) {
        inactiveHz_ = value;
        updateMaximumHz();
    }

    void WaitUntilInitialized() const { initialized_.wait(false); }

    [[nodiscard]] static std::string SuffixedThreadNameFor(const std::string& name) {
        return std::format("{} (GameThread)", name);
    }

    void Start() {
        {
            std::scoped_lock lock{startStopLock_};

            const GameThreadState state = state_;
            if (state != GameThreadState::Paused && state != GameThreadState::NotStarted) {
                throw std::runtime_error(
                    std::format("Cannot start when thread is {}.", magic_enum::enum_name(state)));
            }

            state_ = GameThreadState::Starting;
            PrepareForWork();
        }

        waitForState(GameThreadState::Running);
        assert(state_ == GameThreadState::Running);
    }

    void Exit() {
        std::scoped_lock lock{startStopLock_};

        const GameThreadState state = state_;
        if (state == GameThreadState::Paused || state == GameThreadState::NotStarted ||
            state == GameThreadState::Starting) {
            throw std::runtime_error(
                std::format("Cannot exit when thread is {}.", magic_enum::enum_name(state)));
        }

        if (state == GameThreadState::Exited) {
            return;
        }

        exitRequested_ = true;
    }

  protected:
    bindables::Event<> OnNewFrame;

    virtual void PrepareForWork() {
        assert(!hasThread_);
        createThread();
        assert(hasThread_);
    }

    virtual void OnInitialize() {}

    virtual void OnSuspended() {}

    virtual void OnExit() {}

    virtual void MakeCurrent() {
        development::ThreadSafety::ResetAllForCurrentThread();
        // todo set the synchronization context
    }

    void Initialize(bool withThrottling) {
        std::scoped_lock lock{startStopLock_};

        assert(state_ != GameThreadState::Running);
        assert(state_ != GameThreadState::Exited);

        MakeCurrent();

        OnInitialize();

        clock_->Throttling = withThrottling;

        initialized_ = true;
        initialized_.notify_all();
        state_ = GameThreadState::Running;
    }

  private:
    friend class platform::ThreadRunner;

    void runSingleFrame() {
        if (const auto newState = processFrame()) {
            setExitState(*newState);
        }
    }

    void pause() {
        {
            std::scoped_lock lock{startStopLock_};

            if (state_ != GameThreadState::Running) {
                return;
            }

            pauseRequested_ = true;
        }

        waitForState(GameThreadState::Paused);
    }

    void waitForState(GameThreadState targetState) {
        if (state_ == targetState) {
            return;
        }

        if (!hasThread_) {
            std::optional<GameThreadState> newState;

            while (newState != targetState) {
                newState = processFrame();
            }

            setExitState(*newState);
        } else {
            while (state_ != targetState) {
                std::this_thread::sleep_for(std::chrono::milliseconds{1});
            }
        }

        assert(state_ == targetState);
    }

    [[nodiscard]] bool joinNativeThread(std::chrono::milliseconds timeout) {
        const auto deadline = std::chrono::steady_clock::now() + timeout;

        while (!threadFinished_) {
            if (std::chrono::steady_clock::now() >= deadline) {
                return false;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }

        if (thread_.joinable()) {
            thread_.join();
        }

        return true;
    }

    void updateMaximumHz() {
        const double hz = IsActive->GetValue() ? activeHz_ : inactiveHz_;
        scheduler_->Add([this, hz] { clock_->MaximumUpdateHz = hz; });
    }

    void createThread() {
        assert(!hasThread_);
        assert(!IsRunning());

        if (thread_.joinable()) {
            thread_.join();
        }

        threadFinished_ = false;
        hasThread_ = true;

        // todo name the native thread with SuffixedThreadNameFor(Name)
        thread_ = std::jthread{[this] {
            Initialize(true);

            while (IsRunning()) {
                runSingleFrame();
            }

            threadFinished_ = true;
        }};
    }

    [[nodiscard]] std::optional<GameThreadState> processFrame() {
        if (state_ != GameThreadState::Running) {
            return std::nullopt;
        }

        MakeCurrent();

        if (exitRequested_) {
            exitRequested_ = false;
            return GameThreadState::Exited;
        }

        if (pauseRequested_) {
            pauseRequested_ = false;
            return GameThreadState::Paused;
        }

        try {
            // todo per-frame performance collection and synchronizationContext.RunWork()
            scheduler_->Update();
            OnNewFrame.Invoke();
            clock_->ProcessFrame();
        } catch (...) {
            if (UnhandledException && !development::ThreadSafety::IsInputThread) {
                UnhandledException(std::current_exception());
            } else {
                throw;
            }
        }

        return std::nullopt;
    }

    void setExitState(GameThreadState exitState) {
        std::scoped_lock lock{startStopLock_};

        assert(state_ == GameThreadState::Running);
        assert(exitState == GameThreadState::Exited || exitState == GameThreadState::Paused);

        hasThread_ = false;
        OnSuspended();

        if (exitState == GameThreadState::Exited) {
            // todo dispose Monitor and disassociate the synchronization context
            OnExit();
        }

        state_ = exitState;
    }

    std::shared_ptr<timing::ThrottledFrameClock> clock_;
    std::shared_ptr<Scheduler> scheduler_;

    std::atomic<GameThreadState> state_{GameThreadState::NotStarted};
    double activeHz_ = kDefaultActiveHz;
    double inactiveHz_ = kDefaultInactiveHz;
    std::atomic<bool> initialized_{false};
    std::atomic<bool> pauseRequested_{false};
    std::atomic<bool> exitRequested_{false};
    std::atomic<bool> hasThread_{false};
    std::atomic<bool> threadFinished_{false};

    std::recursive_mutex startStopLock_;
    std::jthread thread_;
};

} // namespace panlid::threading
