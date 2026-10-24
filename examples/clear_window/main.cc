#include "panlid/graphics/Colour4.hh"
#include "panlid/graphics/rendering/ClearInfo.hh"
#include "panlid/graphics/rendering/sdl3/Sdl3GpuRenderer.hh"
#include "panlid/platform/sdl3/Sdl3Window.hh"
#include <exception>
#include <glm/vec2.hpp>
#include <iostream>
#include <memory>

namespace {

using panlid::graphics::Colour4;
using panlid::graphics::rendering::ClearInfo;
using panlid::graphics::rendering::sdl3::Sdl3GpuRenderer;
using panlid::platform::sdl3::Sdl3Window;

// Sdl3Window has a protected constructor, so the demo needs a concrete window type.
class DemoWindow final : public Sdl3Window {
  public:
    DemoWindow() : Sdl3Window("panlid clear_window") {}
};

} // namespace

int main() {
    try {
        DemoWindow window;
        window.Create();
        window.SetTitle("panlid clear_window");

        auto renderer = std::make_unique<Sdl3GpuRenderer>();
        renderer->Initialise(window.GetGraphicsSurface());

        window.AddUpdateHandler([&window, &renderer] {
            renderer->BeginFrame(glm::vec2{window.GetClientSize()});
            renderer->Clear(ClearInfo{.Colour = Colour4::CornflowerBlue()});
            renderer->FinishFrame();
            renderer->SwapBuffers();

            window.OnDraw();
        });

        window.AddExitRequestedHandler([&window] { window.Close(); });

        // The renderer must be destroyed while the window and SDL still exist. The window runs
        // this handler just before it shuts SDL down.
        window.AddExitedHandler([&renderer] { renderer.reset(); });

        window.Run();
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
