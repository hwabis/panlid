#pragma once

#include "panlid/RuntimeInfo.hh"
#include "panlid/bindables/Event.hh"
#include "panlid/logging/Logger.hh"
#include "panlid/platform/IWindow.hh"
#include "panlid/platform/sdl3/Sdl3Extensions.hh"
#include "panlid/platform/sdl3/Sdl3GraphicsSurface.hh"
#include "panlid/threading/Scheduler.hh"
#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_version.h>
#include <SDL3/SDL_video.h>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <glm/vec2.hpp>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace panlid::platform::sdl3 {

// A window using SDL3 for windowing and graphics support. Not thread-safe: use it from the thread
// that runs the window, except for Exists, GetSize, GetClientSize and GetScale.
//
// todo: ISDLWindow once GameHost exists; the windowing and input halves: displays,
// window state and mode, position, resizing, config bindables, safe area and border size, cursor,
// keyboard, joysticks, touch, pen, drag and drop, window icon, flashing.
class Sdl3Window : public IWindow {
  public:
    Sdl3Window(const Sdl3Window&) = delete;
    Sdl3Window(Sdl3Window&&) = delete;
    Sdl3Window& operator=(const Sdl3Window&) = delete;
    Sdl3Window& operator=(Sdl3Window&&) = delete;

    ~Sdl3Window() override {
        Close();

        SDL_RemoveEventWatch(&eventWatch, this);
        SDL_SetEventFilter(nullptr, nullptr);
        SDL_Quit();
    }

    // Whether the window has been created and not yet closed.
    [[nodiscard]] bool Exists() const { return exists_; }

    [[nodiscard]] const std::string& GetTitle() const override { return title_; }

    void SetTitle(std::string title) override {
        title_ = std::move(title);
        ScheduleCommand([this, title = title_] {
            LogErrorIfFailed(SDL_SetWindowTitle(window_, title.c_str()));
        });
    }

    // The native window handle, as provided by the operating system.
    [[nodiscard]] void* GetWindowHandle() const {
        if (window_ == nullptr) {
            return nullptr;
        }

        const SDL_PropertiesID props = SDL_GetWindowProperties(window_);
        if (props == 0) {
            return nullptr;
        }

        switch (RuntimeInfo::kOS) {
        case RuntimeInfo::Platform::Windows:
            return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);

        case RuntimeInfo::Platform::Linux:
            if (isWayland_) {
                return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER,
                                              nullptr);
            }

            if (currentVideoDriver() == "x11") {
                // NOLINTNEXTLINE(performance-no-int-to-ptr)
                return reinterpret_cast<void*>(static_cast<std::uintptr_t>(
                    SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0)));
            }

            return nullptr;

        case RuntimeInfo::Platform::MacOS:
            return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);

        case RuntimeInfo::Platform::IOS:
            return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_UIKIT_WINDOW_POINTER, nullptr);

        case RuntimeInfo::Platform::Android:
            return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER, nullptr);
        }

        throw std::out_of_range("Unknown platform.");
    }

    // The display connection handle. Only available on Linux.
    [[nodiscard]] void* GetDisplayHandle() const {
        if (window_ == nullptr) {
            return nullptr;
        }

        const SDL_PropertiesID props = SDL_GetWindowProperties(window_);
        if (props == 0) {
            return nullptr;
        }

        if (isWayland_) {
            return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
        }

        if (currentVideoDriver() == "x11") {
            return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
        }

        return nullptr;
    }

    // todo: IWindow keeps this protected for GameHost. Make it protected again once GameHost exists
    [[nodiscard]] Sdl3GraphicsSurface& GetGraphicsSurface() override { return graphicsSurface_; }

    void Create() override {
        // The window starts hidden and is shown after the first swap, to avoid a white flash.
        // todo: add the window state flags.
        const SDL_WindowFlags flags =
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_HIDDEN;

        LogErrorIfFailed(SDL_SetHint(SDL_HINT_WINDOWS_CLOSE_ON_ALT_F4, "0"));
        LogErrorIfFailed(SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_MODE_CENTER, "0"));
        // Disable touch events generating synthetic mouse events on desktop platforms.
        LogErrorIfFailed(SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0"));
        // Disable mouse events generating synthetic touch events on mobile platforms.
        LogErrorIfFailed(SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0"));
        LogErrorIfFailed(SDL_SetHint(SDL_HINT_PEN_TOUCH_EVENTS, "0"));
        LogErrorIfFailed(SDL_SetHint(SDL_HINT_PEN_MOUSE_EVENTS, "0"));
        LogErrorIfFailed(SDL_SetHint(SDL_HINT_IME_IMPLEMENTED_UI, "composition"));

        const glm::ivec2 size = GetSize();
        window_ = SDL_CreateWindow(title_.c_str(), size.x, size.y, flags);

        if (window_ == nullptr) {
            throw std::runtime_error(
                std::format("Failed to create SDL window: {}", SDL_GetError()));
        }

        // Text input should only be active while a text input handler wants it, but SDL turns it
        // on by default on some platforms, so turn it off at startup.
        LogErrorIfFailed(SDL_StopTextInput(window_));

        graphicsSurface_.Initialise();

        // todo: the rest of the windowing setup (displays, window state, config).
        fetchWindowSize();
        exists_ = true;
    }

    // Starts the window's run loop.
    void Run() override {
        LogErrorIfFailed(SDL_AddEventWatch(&eventWatch, this));

        RunMainLoop();
    }

    // Shows the window after the first draw, to avoid a white flash on startup.
    void OnDraw() override {
        if (!firstDraw_) {
            return;
        }

        SetVisible(true);
        firstDraw_ = false;
    }

    // Forcefully closes the window.
    void Close() override {
        if (Exists()) {
            // Close is called again when the run loop finishes.
            ScheduleCommand([this] { exists_ = false; });
        } else if (window_ != nullptr) {
            SDL_DestroyWindow(window_);
            window_ = nullptr;
        }
    }

    void Raise() override {
        ScheduleCommand([this] {
            if ((SDL_GetWindowFlags(window_) & SDL_WINDOW_MINIMIZED) != 0) {
                LogErrorIfFailed(SDL_RestoreWindow(window_));
            }

            LogErrorIfFailed(SDL_RaiseWindow(window_));
        });
    }

    void Hide() override {
        ScheduleCommand([this] { LogErrorIfFailed(SDL_HideWindow(window_)); });
    }

    void Show() override {
        ScheduleCommand([this] { LogErrorIfFailed(SDL_ShowWindow(window_)); });
    }

    // todo: Flash, which needs the window's active state.

    void CancelFlash() override {
        ScheduleCommand([this] {
            if (!RuntimeInfo::kIsDesktop) {
                return;
            }

            LogErrorIfFailed(SDL_FlashWindow(window_, SDL_FLASH_CANCEL));
        });
    }

    void EnableScreenSuspension() override {
        ScheduleCommand([] { LogErrorIfFailed(SDL_EnableScreenSaver()); });
    }

    void DisableScreenSuspension() override {
        ScheduleCommand([] { LogErrorIfFailed(SDL_DisableScreenSaver()); });
    }

    [[nodiscard]] bool IsVisible() const { return visible_; }

    void SetVisible(bool visible) {
        visible_ = visible;
        ScheduleCommand([this, visible] {
            if (visible) {
                LogErrorIfFailed(SDL_ShowWindow(window_));
            } else {
                LogErrorIfFailed(SDL_HideWindow(window_));
            }
        });
    }

    // The window's internal size, before scaling.
    [[nodiscard]] glm::ivec2 GetSize() const override {
        std::scoped_lock lock{sizeLock_};
        return size_;
    }

    // The drawable area, after scaling.
    [[nodiscard]] glm::ivec2 GetClientSize() const override {
        std::scoped_lock lock{sizeLock_};
        return {static_cast<int>(static_cast<float>(size_.x) * scale_),
                static_cast<int>(static_cast<float>(size_.y) * scale_)};
    }

    [[nodiscard]] float GetScale() const override {
        std::scoped_lock lock{sizeLock_};
        return scale_;
    }

    // Invoked once every window event loop.
    void AddUpdateHandler(bindables::Event<>::Handler handler) override {
        update_ += std::move(handler);
    }

    // Invoked when the application has been suspended.
    void AddSuspendedHandler(bindables::Event<>::Handler handler) override {
        suspended_ += std::move(handler);
    }

    // Invoked when the application has been resumed from suspension.
    void AddResumedHandler(bindables::Event<>::Handler handler) override {
        resumed_ += std::move(handler);
    }

    // Invoked when the operating system is low on memory.
    void AddLowOnMemoryHandler(bindables::Event<>::Handler handler) override {
        lowOnMemory_ += std::move(handler);
    }

    // Invoked when the window close button or another platform exit action has been used.
    void AddExitRequestedHandler(bindables::Event<>::Handler handler) override {
        exitRequested_ += std::move(handler);
    }

    // Invoked when the window is about to close.
    void AddExitedHandler(bindables::Event<>::Handler handler) override {
        exited_ += std::move(handler);
    }

    // Invoked after the window has resized.
    void AddResizedHandler(bindables::Event<>::Handler handler) override {
        resized_ += std::move(handler);
    }

  protected:
    explicit Sdl3Window(const std::string& appName) {
        LogErrorIfFailed(SDL_SetHint(SDL_HINT_APP_NAME, appName.c_str()));

        // todo: SDL_INIT_GAMEPAD, once joysticks are ported.
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            throw std::runtime_error(std::format("Failed to initialise SDL: {}", SDL_GetError()));
        }

        const int version = SDL_GetVersion();
        logging::Logger::Log(
            std::format("SDL3 Initialized\n"
                        "SDL3 Version: {}.{}.{}\n"
                        "SDL3 Revision: {}\n"
                        "SDL3 Video driver: {}",
                        SDL_VERSIONNUM_MAJOR(version), SDL_VERSIONNUM_MINOR(version),
                        SDL_VERSIONNUM_MICRO(version), SDL_GetRevision(), currentVideoDriver()));

        isWayland_ = currentVideoDriver() == "wayland";

        SDL_SetLogOutputFunction(&logOutput, nullptr);
        SDL_SetEventFilter(&eventFilter, this);

        // todo: cursor state changes and populating joysticks.
    }

    // Blocks, running frames for as long as the window exists, then runs the cleanup.
    virtual void RunMainLoop() {
        while (Exists()) {
            RunFrame();
        }

        exited_.Invoke();
        Close();
        SDL_Quit();
    }

    void RunFrame() {
        commandScheduler_.Update();

        if (!Exists()) {
            return;
        }

        // todo: apply a pending window state change, and poll the mouse while the cursor is
        // outside the window.
        pollSdlEvents();

        eventScheduler_.Update();
        update_.Invoke();
    }

    void SetSize(glm::ivec2 size) {
        {
            std::scoped_lock lock{sizeLock_};

            if (size == size_) {
                return;
            }

            size_ = size;
        }

        resized_.Invoke();
    }

    // Handles events from SDL's event filter. Return false to drop the event.
    virtual bool HandleEventFromFilter(const SDL_Event& e) {
        switch (e.type) {
        case SDL_EVENT_TERMINATING:
            handleQuitEvent();
            break;

        case SDL_EVENT_DID_ENTER_BACKGROUND:
            suspended_.Invoke();
            break;

        case SDL_EVENT_WILL_ENTER_FOREGROUND:
            resumed_.Invoke();
            break;

        case SDL_EVENT_LOW_MEMORY:
            lowOnMemory_.Invoke();
            break;

        // todo: mouse motion, button and wheel events are handled here and dropped (return false).
        default:
            break;
        }

        return true;
    }

    void HandleEventFromWatch(const SDL_Event& e) {
        switch (e.type) {
        case SDL_EVENT_WINDOW_RESIZED:
            // Polling blocks while the user resizes the window, so resizes are handled here.
            // todo: skip while a window state change is in progress, and store the size to config
            // when the user is resizing.
            fetchWindowSize();
            break;

        default:
            break;
        }
    }

    // Handles events polled on the main thread.
    virtual void HandleEvent(const SDL_Event& e) {
        // todo: display events.
        if (e.type >= SDL_EVENT_WINDOW_FIRST && e.type <= SDL_EVENT_WINDOW_LAST) {
            handleWindowEvent(e.window);
            return;
        }

        switch (e.type) {
        case SDL_EVENT_QUIT:
            handleQuitEvent();
            break;

        // todo: keyboard, text, joystick, gamepad, touch, drop and pen events.
        default:
            break;
        }
    }

    // Runs the action at the end of the current event loop.
    void ScheduleEvent(std::function<void()> action) {
        eventScheduler_.Add(std::move(action), false);
    }

    // Runs the action before the next event loop.
    void ScheduleCommand(std::function<void()> action) {
        commandScheduler_.Add(std::move(action), false);
    }

  private:
    friend class Sdl3GraphicsSurface;

    static constexpr int kDefaultWidth = 1366;
    static constexpr int kDefaultHeight = 768;
    static constexpr int kEventsPerPeep = 64;

    SDL_Window* window_ = nullptr;
    Sdl3GraphicsSurface graphicsSurface_{*this};
    std::atomic<bool> exists_ = false;
    bool isWayland_ = false;
    bool visible_ = false;
    bool firstDraw_ = true;
    std::string title_;

    mutable std::mutex sizeLock_;
    glm::ivec2 size_{kDefaultWidth, kDefaultHeight};
    float scale_ = 1.0F;

    threading::Scheduler commandScheduler_;
    threading::Scheduler eventScheduler_;
    std::array<SDL_Event, kEventsPerPeep> events_{};

    bindables::Event<> update_;
    bindables::Event<> suspended_;
    bindables::Event<> resumed_;
    bindables::Event<> lowOnMemory_;
    bindables::Event<> exitRequested_;
    bindables::Event<> exited_;
    bindables::Event<> resized_;

    [[nodiscard]] static std::string_view currentVideoDriver() {
        const char* driver = SDL_GetCurrentVideoDriver();
        return driver != nullptr ? driver : "";
    }

    static void logOutput(void* /*userdata*/, int category, SDL_LogPriority priority,
                          const char* message) {
        logging::Logger::Log(
            std::format("SDL {} log [{}]: {}", ReadableName(static_cast<SDL_LogCategory>(category)),
                        ReadableName(priority), message != nullptr ? message : ""));
    }

    static bool eventFilter(void* userdata, SDL_Event* event) {
        return static_cast<Sdl3Window*>(userdata)->HandleEventFromFilter(*event);
    }

    static bool eventWatch(void* userdata, SDL_Event* event) {
        static_cast<Sdl3Window*>(userdata)->HandleEventFromWatch(*event);
        return true;
    }

    [[nodiscard]] SDL_Window* getSdlWindowHandle() const { return window_; }

    void fetchWindowSize() {
        int width = 0;
        int height = 0;
        LogErrorIfFailed(SDL_GetWindowSize(window_, &width, &height));

        const int drawableWidth = graphicsSurface_.GetDrawableSize().x;

        // When minimised on Windows, these values may be zero. Ignore them.
        if (width <= 0 || drawableWidth <= 0) {
            return;
        }

        {
            std::scoped_lock lock{sizeLock_};
            scale_ = static_cast<float>(drawableWidth) / static_cast<float>(width);
        }

        SetSize({width, height});

        // todo: store the window size to config.
    }

    void pollSdlEvents() {
        SDL_PumpEvents();

        int eventsRead = 0;

        do {
            eventsRead = LogErrorIfFailed(SDL_PeepEvents(
                events_.data(), kEventsPerPeep, SDL_GETEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST));

            for (int i = 0; i < eventsRead; ++i) {
                HandleEvent(events_[static_cast<std::size_t>(i)]);
            }
        } while (eventsRead == kEventsPerPeep);
    }

    void handleWindowEvent(const SDL_WindowEvent& evt) {
        // todo: refresh the window state before handling the event.
        switch (evt.type) {
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            fetchWindowSize();
            break;

        // todo: moved, mouse enter and leave, focus, restored and minimised, and refreshing
        // displays.
        default:
            break;
        }
    }

    void handleQuitEvent() { exitRequested_.Invoke(); }
};

} // namespace panlid::platform::sdl3
