#pragma once

#include "panlid/platform/IGraphicsSurface.hh"
#include <SDL3/SDL.h>
#include <glm/vec2.hpp>

namespace panlid::platform::sdl3 {

class Sdl3Window;

// The graphics surface of an SDL3 window.
// todo: OpenGL, Metal, Linux and Android surface interfaces; only the SDL3 GPU backend is targeted.
class Sdl3GraphicsSurface final : public IGraphicsSurface {
  public:
    explicit Sdl3GraphicsSurface(Sdl3Window& window);

    [[nodiscard]] void* GetWindowHandle() const override;

    // The SDL GPU device is created by the renderer, so there is nothing to initialise here.
    void Initialise() override {}

    [[nodiscard]] glm::ivec2 GetDrawableSize() const override;

    // Not part of IGraphicsSurface: the SDL GPU renderer needs the SDL window to claim it.
    [[nodiscard]] SDL_Window* GetSdlWindowHandle() const;

  private:
    Sdl3Window& window_;
};

} // namespace panlid::platform::sdl3
