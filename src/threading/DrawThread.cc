#include "panlid/threading/DrawThread.hh"
#include "panlid/graphics/rendering/Renderer.hh"
#include "panlid/platform/GameHost.hh"
#include "panlid/platform/IWindow.hh"
#include <glm/vec2.hpp>

namespace panlid::threading {

void DrawThread::OnInitialize() {
    platform::IWindow* window = host_.GetWindow();

    if (window != nullptr) {
        graphics::rendering::Renderer& renderer = *host_.GetRenderer();

        renderer.BeginFrame(glm::vec2{window->GetClientSize()});
        renderer.FinishFrame();
    }
}

void DrawThread::OnSuspended() {
    GameThread::OnSuspended();

    // Release the renderer's hold on this thread before suspending.
    graphics::rendering::Renderer* renderer = host_.GetRenderer();

    if (renderer != nullptr && renderer->IsInitialised()) {
        renderer->ClearCurrent();
    }
}

} // namespace panlid::threading
