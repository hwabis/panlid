#pragma once

#include "panlid/bindables/Event.hh"
#include "panlid/platform/IGraphicsSurface.hh"
#include <glm/vec2.hpp>
#include <string>

namespace panlid::platform {

// todo: remove this forward declaration once GameHost exists.
class GameHost;

// The game window, hiding any implementation-specific code.
//
// todo: CycleMode, SetupWindow, KeymapChanged, DragDrop, cursor state and confinement, window state
// and mode, displays, safe area, border size, Flash, SetIconFromStream, position, minimum and
// maximum size, resizable.
class IWindow {
  public:
    IWindow() = default;
    IWindow(const IWindow&) = delete;
    IWindow(IWindow&&) = delete;
    IWindow& operator=(const IWindow&) = delete;
    IWindow& operator=(IWindow&&) = delete;
    virtual ~IWindow() = default;

    // Creates the concrete window implementation.
    virtual void Create() = 0;

    // Starts the window's run loop. Blocks on desktop platforms, and returns immediately on mobile
    // platforms.
    virtual void Run() = 0;

    // Called once a draw session has finished.
    virtual void OnDraw() = 0;

    // Forcefully closes the window.
    virtual void Close() = 0;

    // Called once every window event loop.
    virtual void AddUpdateHandler(bindables::Event<>::Handler handler) = 0;

    // Called when the close button or another platform exit action has been used.
    virtual void AddExitRequestedHandler(bindables::Event<>::Handler handler) = 0;

    // Called when the window has closed.
    virtual void AddExitedHandler(bindables::Event<>::Handler handler) = 0;

    // Called when the application has been suspended.
    virtual void AddSuspendedHandler(bindables::Event<>::Handler handler) = 0;

    // Called when the application has been resumed from suspension.
    virtual void AddResumedHandler(bindables::Event<>::Handler handler) = 0;

    // Called when the operating system is low on memory, so the application can free some.
    virtual void AddLowOnMemoryHandler(bindables::Event<>::Handler handler) = 0;

    // Called when the client size has changed.
    virtual void AddResizedHandler(bindables::Event<>::Handler handler) = 0;

    // Tries to raise the window above other windows and request input focus.
    virtual void Raise() = 0;

    // Tries to hide the window, making it invisible and hidden from the taskbar.
    virtual void Hide() = 0;

    // Tries to show the window, making it visible.
    virtual void Show() = 0;

    // Tries to cancel a window flash. Does nothing on platforms without flashing.
    virtual void CancelFlash() = 0;

    // Enables any system timers that might dim or turn off the screen.
    virtual void EnableScreenSuspension() = 0;

    // Disables any system timers that might dim or turn off the screen.
    virtual void DisableScreenSuspension() = 0;

    [[nodiscard]] virtual const std::string& GetTitle() const = 0;
    virtual void SetTitle(std::string title) = 0;

    // The size of the window in scaled pixels, excluding any decoration or border.
    [[nodiscard]] virtual glm::ivec2 GetSize() const = 0;

    // The size of the window in pixels, excluding any decoration or border.
    [[nodiscard]] virtual glm::ivec2 GetClientSize() const = 0;

    // The ratio of the client size to the size.
    [[nodiscard]] virtual float GetScale() const = 0;

  protected:
    // The surface the renderer draws to.
    [[nodiscard]] virtual IGraphicsSurface& GetGraphicsSurface() = 0;

  private:
    friend class GameHost;
};

} // namespace panlid::platform
