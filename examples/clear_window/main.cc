#include "panlid/graphics/rendering/Renderer.hh"
#include "panlid/graphics/rendering/sdl3/Sdl3GpuRenderer.hh"
#include "panlid/logging/Logger.hh"
#include "panlid/platform/GameHost.hh"
#include "panlid/platform/IWindow.hh"
#include "panlid/platform/sdl3/Sdl3Window.hh"
#include <cstddef>
#include <exception>
#include <format>
#include <functional>
#include <iostream>
#include <memory>
#include <thread>

namespace {

using panlid::graphics::rendering::Renderer;
using panlid::graphics::rendering::sdl3::Sdl3GpuRenderer;
using panlid::logging::Logger;
using panlid::platform::GameHost;
using panlid::platform::IWindow;
using panlid::platform::sdl3::Sdl3Window;

// A number that is the same for the same thread and different for different threads.
[[nodiscard]] std::size_t CurrentThreadNumber() {
    return std::hash<std::thread::id>{}(std::this_thread::get_id());
}

// Sdl3Window has a protected constructor, so the demo needs a concrete window type.
class DemoWindow final : public Sdl3Window {
  public:
    DemoWindow() : Sdl3Window("panlid clear_window") {}
};

class DemoHost final : public GameHost {
  public:
    DemoHost() : GameHost{"clear_window"} {}

  protected:
    std::unique_ptr<IWindow> CreateWindow() override { return std::make_unique<DemoWindow>(); }

    std::unique_ptr<Renderer> CreateRenderer() override {
        return std::make_unique<Sdl3GpuRenderer>();
    }

    void UpdateFrame() override {
        if (!loggedUpdate_) {
            loggedUpdate_ = true;
            Logger::Log(std::format("update frame on thread {}", CurrentThreadNumber()));
        }

        GameHost::UpdateFrame();
    }

    void DrawFrame() override {
        if (!loggedDraw_) {
            loggedDraw_ = true;
            Logger::Log(std::format("draw frame on thread {}", CurrentThreadNumber()));
        }

        GameHost::DrawFrame();
    }

  private:
    bool loggedUpdate_ = false;
    bool loggedDraw_ = false;
};

} // namespace

int main() {
    try {
        Logger::Log(std::format("main thread {}", CurrentThreadNumber()));

        DemoHost host;

        // With no game to ask the host to exit, the demo does it.
        host.AddExitRequestedHandler([&host] { host.Exit(); });

        host.Run();
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
