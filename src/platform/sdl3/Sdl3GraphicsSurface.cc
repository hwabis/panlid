#include "panlid/platform/sdl3/Sdl3GraphicsSurface.hh"
#include "panlid/platform/sdl3/Sdl3Extensions.hh"
#include "panlid/platform/sdl3/Sdl3Window.hh"
#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>
#include <glm/vec2.hpp>

namespace panlid::platform::sdl3 {

Sdl3GraphicsSurface::Sdl3GraphicsSurface(Sdl3Window& window) : window_{window} {}

void* Sdl3GraphicsSurface::GetWindowHandle() const { return window_.GetWindowHandle(); }

glm::ivec2 Sdl3GraphicsSurface::GetDrawableSize() const {
    int width = 0;
    int height = 0;
    ThrowIfFailed(SDL_GetWindowSizeInPixels(window_.getSdlWindowHandle(), &width, &height));
    return {width, height};
}

SDL_Window* Sdl3GraphicsSurface::GetSdlWindowHandle() const { return window_.getSdlWindowHandle(); }

} // namespace panlid::platform::sdl3
