#include "panlid/graphics/rendering/Renderer.hh"
#include "panlid/graphics/rendering/sdl3/Sdl3GpuRenderer.hh"
#include "panlid/platform/GameHost.hh"
#include "panlid/platform/IWindow.hh"
#include "panlid/platform/sdl3/Sdl3Window.hh"
#include <exception>
#include <iostream>
#include <memory>

namespace {

using panlid::graphics::rendering::Renderer;
using panlid::graphics::rendering::sdl3::Sdl3GpuRenderer;
using panlid::platform::GameHost;
using panlid::platform::IWindow;
using panlid::platform::sdl3::Sdl3Window;

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
};

} // namespace

int main() {
    try {
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
