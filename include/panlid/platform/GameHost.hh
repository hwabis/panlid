#pragma once

#include "panlid/HostOptions.hh"
#include "panlid/bindables/Bindable.hh"
#include "panlid/bindables/Event.hh"
#include "panlid/bindables/ValueChangedEvent.hh"
#include "panlid/graphics/rendering/Renderer.hh"
#include "panlid/logging/Logger.hh"
#include "panlid/platform/ExecutionMode.hh"
#include "panlid/platform/ExecutionState.hh"
#include "panlid/platform/IWindow.hh"
#include "panlid/platform/ThreadRunner.hh"
#include "panlid/threading/DrawThread.hh"
#include "panlid/threading/GameThread.hh"
#include "panlid/threading/InputThread.hh"
#include "panlid/threading/UpdateThread.hh"
#include <atomic>
#include <cassert>
#include <chrono>
#include <exception>
#include <format>
#include <glm/vec2.hpp>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace panlid::platform {

// Hosts a game: owns the window, the renderer and the threads, and runs them.
//
// todo: Game and the scene graph, config managers, storage, IPC, input handlers, clipboard, text
// input, file selector, opening files and URLs externally, platform key bindings, AudioThread,
// PerformanceMonitor, screenshots, window mode, display and frame sync handling, unhandled
// exception handling, AllowScreenSuspension, IsPrimaryInstance, RendererInfo and choosing between
// renderers, Collect, resource store factories.
class GameHost {
  public:
    explicit GameHost(std::string gameName, HostOptions options = {})
        : name_{std::move(gameName)}, options_{std::move(options)} {
        if (options_.FriendlyGameName.empty()) {
            options_.FriendlyGameName = std::format("panlid (running \"{}\")", name_);
        }

        // todo: default JSON settings (the Vector2 converter).
    }

    GameHost(const GameHost&) = delete;
    GameHost(GameHost&&) = delete;
    GameHost& operator=(const GameHost&) = delete;
    GameHost& operator=(GameHost&&) = delete;

    virtual ~GameHost() {
        // todo: wait for the host to finish stopping, once Run can be called from another thread.
        assert(executionState_ != ExecutionState::Running);
    }

    // Null when running headless.
    [[nodiscard]] IWindow* GetWindow() const { return window_.get(); }

    [[nodiscard]] graphics::rendering::Renderer* GetRenderer() const { return renderer_.get(); }

    // Whether the window is active (in the foreground).
    const std::shared_ptr<bindables::Bindable<bool>> IsActive =
        bindables::Bindable<bool>::Create(true);

    // Called when the game window is activated. Always called from the update thread.
    void AddActivatedHandler(bindables::Event<>::Handler handler) {
        activated_ += std::move(handler);
    }

    // Called when the game window is deactivated. Always called from the update thread.
    void AddDeactivatedHandler(bindables::Event<>::Handler handler) {
        deactivated_ += std::move(handler);
    }

    // Called when an exit was requested, usually by the window close button or another platform
    // exit action. Always called from the update thread.
    void AddExitRequestedHandler(bindables::Event<>::Handler handler) {
        exitRequested_ += std::move(handler);
    }

    void AddExitedHandler(bindables::Event<>::Handler handler) { exited_ += std::move(handler); }

    // Whether this host can exit. Mobile platforms can't.
    [[nodiscard]] virtual bool CanExit() const { return true; }

    // Whether this host can suspend and minimise to the background. An alternative to exiting on
    // hosts that can't exit.
    [[nodiscard]] virtual bool CanSuspendToBackground() const { return false; }

    [[nodiscard]] std::vector<std::shared_ptr<threading::GameThread>> GetThreads() const {
        return threadRunner_->GetThreads();
    }

    // Registers a thread to be run and tracked by this host.
    void RegisterThread(const std::shared_ptr<threading::GameThread>& thread) {
        threadRunner_->AddThread(thread);

        // todo: bind the thread's IsActive to ours once IWindow exposes it, set its
        // UnhandledException, and enable performance profiling.
    }

    void UnregisterThread(const std::shared_ptr<threading::GameThread>& thread) {
        threadRunner_->RemoveThread(thread);
    }

    [[nodiscard]] const std::shared_ptr<threading::DrawThread>& GetDrawThread() const {
        return drawThread_;
    }

    [[nodiscard]] const std::shared_ptr<threading::UpdateThread>& GetUpdateThread() const {
        return updateThread_;
    }

    [[nodiscard]] const std::shared_ptr<threading::InputThread>& GetInputThread() const {
        return inputThread_;
    }

    // The target number of update frames per second when the window is active. 0 means unlimited.
    [[nodiscard]] double GetMaximumUpdateHz() const { return maximumUpdateHz_; }

    void SetMaximumUpdateHz(double value) {
        assert(updateThread_ != nullptr);

        maximumUpdateHz_ = value;
        updateThread_->SetActiveHz(value);
        threadRunner_->SetMaximumUpdateHz(value);
    }

    // The target number of draw frames per second when the window is active. 0 means unlimited.
    [[nodiscard]] double GetMaximumDrawHz() const { return maximumDrawHz_; }

    void SetMaximumDrawHz(double value) {
        maximumDrawHz_ = value;

        if (drawThread_ != nullptr) {
            drawThread_->SetActiveHz(value);
        }
    }

    // The target number of frames per second on every thread when the window is inactive. 0 means
    // unlimited.
    [[nodiscard]] double GetMaximumInactiveHz() const { return maximumInactiveHz_; }

    void SetMaximumInactiveHz(double value) {
        assert(updateThread_ != nullptr);

        maximumInactiveHz_ = value;
        updateThread_->SetInactiveHz(value);
        threadRunner_->SetMaximumInactiveHz(value);

        if (drawThread_ != nullptr) {
            drawThread_->SetInactiveHz(value);
        }
    }

    // The name of the game to be hosted.
    [[nodiscard]] const std::string& GetName() const { return name_; }

    [[nodiscard]] const HostOptions& GetOptions() const { return options_; }

    [[nodiscard]] ExecutionState GetExecutionState() const { return executionState_; }

    // Schedules the game to exit in the next frame.
    void Exit() {
        if (CanExit()) {
            PerformExit(false);
        }
    }

    // Suspends and minimises the game to the background. Returns whether it succeeded. Only call
    // this if CanSuspendToBackground is true.
    virtual bool SuspendToBackground() { return false; }

    // Runs the host. Blocks on desktop platforms.
    void Run() {
        if (executionState_ != ExecutionState::Idle) {
            throw std::runtime_error("A game that has already been run cannot be restarted.");
        }

        // todo: fail when run on a thread pool thread.
        if (!HostRunningMutex.try_lock_for(std::chrono::seconds{10})) {
            throw std::runtime_error(std::format(
                "This GameHost could not start {} because another GameHost was already running.",
                name_));
        }

        try {
            runLocked();
        } catch (...) {
            finishRun();
            throw;
        }

        finishRun();
    }

    // Pauses all active threads. Resume continues them.
    void Suspend() {
        suspended_ = true;
        threadRunner_->Suspend();
    }

    void Resume() {
        threadRunner_->Start();
        suspended_ = false;
    }

  protected:
    // Whether the window must exist for startup to succeed.
    [[nodiscard]] virtual bool IsWindowRequired() const { return true; }

    // Creates the game window. May return null when running headless.
    // todo: take the preferred graphics surface type.
    virtual std::unique_ptr<IWindow> CreateWindow() = 0;

    // Creates the renderer. Replaces choosing between renderers while there is only one backend.
    virtual std::unique_ptr<graphics::rendering::Renderer> CreateRenderer() = 0;

    virtual std::unique_ptr<ThreadRunner>
    CreateThreadRunner(std::shared_ptr<threading::InputThread> mainThread) {
        return std::make_unique<ThreadRunner>(std::move(mainThread));
    }

    virtual void OnActivated() {
        updateThread_->GetScheduler()->Add([this] { activated_.Invoke(); });
    }

    virtual void OnDeactivated() {
        updateThread_->GetScheduler()->Add([this] { deactivated_.Invoke(); });
    }

    virtual void OnExitRequested() {
        updateThread_->GetScheduler()->Add([this] { exitRequested_.Invoke(); });
    }

    virtual void OnExited() { exited_.Invoke(); }

    virtual void UpdateFrame() {
        // todo: update and generate draw nodes for the scene graph, once Game and Root exist.
    }

    virtual void DrawFrame() {
        assert(window_ != nullptr);

        // todo: return while there is no scene graph root or the window is minimised, allow tearing
        // in fullscreen, and read the draw nodes the update thread produced.
        if (executionState_ != ExecutionState::Running) {
            return;
        }

        // Only wait if a frame was rendered since the last wait. Waiting otherwise can leave the
        // wait handle in a bad state, so it takes its full timeout to recover.
        if (didRenderFrame_) {
            renderer_->WaitUntilNextFrameReady();
        }

        didRenderFrame_ = false;

        renderer_->BeginFrame(glm::vec2{window_->GetClientSize()});

        // todo: the front and back draw passes.

        renderer_->FinishFrame();

        Swap();

        window_->OnDraw();
        didRenderFrame_ = true;
    }

    virtual void Swap() { renderer_->SwapBuffers(); }

    // Schedules the game to exit in the next frame, or immediately if asked. Never called if
    // CanExit is false.
    virtual void PerformExit(bool immediately) { performExit(immediately); }

    void SetupRendererAndWindow() {
        logging::Logger::Log("Initialising renderer");

        renderer_ = CreateRenderer();

        // todo: the renderer's shader cache storage.
        try {
            window_ = CreateWindow();

            if (window_ == nullptr) {
                if (!IsWindowRequired()) {
                    return;
                }

                throw std::runtime_error(
                    "Renderer could not be initialised as window creation failed.");
            }

            // todo: set the window up from the config.
            window_->Create();
            window_->SetTitle(options_.FriendlyGameName);

            renderer_->Initialise(window_->GetGraphicsSurface());

            logging::Logger::Log("Renderer initialised!");
        } catch (const std::exception& e) {
            logging::Logger::Log("Renderer initialisation failed with:");
            logging::Logger::Log(e.what());

            renderer_.reset();

            if (window_ != nullptr) {
                window_->Close();
                window_.reset();
            }

            throw;
        }

        // todo: follow the display mode and the current display, bind IsActive to the window's, and
        // apply AllowScreenSuspension.
    }

  private:
    static inline std::timed_mutex HostRunningMutex;

    std::string name_;
    HostOptions options_;

    // The window is declared before the renderer so that the renderer is destroyed first.
    std::unique_ptr<IWindow> window_;
    std::unique_ptr<graphics::rendering::Renderer> renderer_;
    std::unique_ptr<ThreadRunner> threadRunner_;
    std::shared_ptr<threading::InputThread> inputThread_;
    std::shared_ptr<threading::UpdateThread> updateThread_;
    std::shared_ptr<threading::DrawThread> drawThread_;

    double maximumUpdateHz_ = threading::GameThread::kDefaultActiveHz;
    double maximumDrawHz_ = threading::GameThread::kDefaultActiveHz;
    double maximumInactiveHz_ = threading::GameThread::kDefaultInactiveHz;

    bindables::Event<> activated_;
    bindables::Event<> deactivated_;
    bindables::Event<> exitRequested_;
    bindables::Event<> exited_;

    std::atomic<ExecutionState> executionState_{ExecutionState::Idle};
    bool didRenderFrame_ = false;
    bool suspended_ = false;

    void finishRun() {
        if (CanExit()) {
            // Closes the window and stops all threads.
            performExit(true);

            HostRunningMutex.unlock();
        }
    }

    void runLocked() {
        inputThread_ = std::make_shared<threading::InputThread>();
        threadRunner_ = CreateThreadRunner(inputThread_);

        // todo: unhandled and unobserved exception handlers.
        RegisterThread(inputThread_);

        // todo: AudioThread.

        // todo: the draw thread does not exist yet here, so the update thread never waits for it.
        // Decide whether it should.
        updateThread_ =
            std::make_shared<threading::UpdateThread>([this] { UpdateFrame(); }, drawThread_);
        RegisterThread(updateThread_);

        // todo: logger identifiers, storage, config, input handlers.

        SetupRendererAndWindow();

        if (IsWindowRequired() && window_ == nullptr) {
            logging::Logger::Log("Aborting startup as no window could be created.");
            return;
        }

        drawThread_ = std::make_shared<threading::DrawThread>([this] { DrawFrame(); }, *this);
        drawThread_->SetActiveHz(maximumDrawHz_);
        drawThread_->SetInactiveHz(maximumInactiveHz_);
        RegisterThread(drawThread_);

        // todo: key combination provider, text input, clipboard.

        // todo: read the execution mode from the config.
        threadRunner_->SetExecutionMode(ExecutionMode::MultiThreaded);

        setExecutionState(ExecutionState::Running);
        threadRunner_->Start();

        drawThread_->WaitUntilInitialized();

        // todo: bootstrap the scene graph, and apply the frame sync mode.

        IsActive->BindValueChanged(
            [this](const bindables::ValueChangedEvent<bool>&) {
                if (IsActive->GetValue()) {
                    OnActivated();
                } else {
                    OnDeactivated();
                }
            },
            true);

        if (window_ != nullptr) {
            window_->AddUpdateHandler([this] { windowUpdate(); });
            window_->AddSuspendedHandler([this] { Suspend(); });
            window_->AddResumedHandler([this] { Resume(); });
            // todo: free caches when the window reports low memory.
            window_->AddExitRequestedHandler([this] { OnExitRequested(); });
            window_->AddExitedHandler([this] { OnExited(); });
            // todo: key map changes.

            // All threads must stop before the window closes, so nothing draws after cleanup.
            // The renderer must go before the window and SDL do.
            window_->AddExitedHandler([this] {
                threadRunner_->Stop();
                renderer_.reset();
            });

            window_->Run();
        } else {
            while (executionState_ != ExecutionState::Stopped) {
                windowUpdate();
            }
        }
    }

    void windowUpdate() {
        // todo: end and start the input performance collection periods.
        if (suspended_) {
            return;
        }

        threadRunner_->RunMainLoop();
    }

    void setExecutionState(ExecutionState value) {
        if (executionState_ == value) {
            return;
        }

        executionState_ = value;
        logging::Logger::Log(
            std::format("Host execution state changed to {}", magic_enum::enum_name(value)));
    }

    void performExit(bool immediately) {
        if (executionState_ == ExecutionState::Stopped || executionState_ == ExecutionState::Idle) {
            return;
        }

        setExecutionState(ExecutionState::Stopping);

        if (immediately) {
            finishExit();
        } else {
            inputThread_->GetScheduler()->Add([this] { finishExit(); }, false);
        }
    }

    void finishExit() {
        assert(executionState_ == ExecutionState::Stopping);

        if (window_ != nullptr) {
            window_->Close();
        }

        threadRunner_->Stop();

        setExecutionState(ExecutionState::Stopped);

        // todo: signal that the host has stopped.
    }
};

} // namespace panlid::platform
