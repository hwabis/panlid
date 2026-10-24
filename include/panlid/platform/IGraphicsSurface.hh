#pragma once

#include <glm/vec2.hpp>

namespace panlid::platform {

// The graphics API surface a window provides.
class IGraphicsSurface {
  public:
    IGraphicsSurface() = default;
    IGraphicsSurface(const IGraphicsSurface&) = delete;
    IGraphicsSurface(IGraphicsSurface&&) = delete;
    IGraphicsSurface& operator=(const IGraphicsSurface&) = delete;
    IGraphicsSurface& operator=(IGraphicsSurface&&) = delete;
    virtual ~IGraphicsSurface() = default;

    // The operating system's handle to the window.
    [[nodiscard]] virtual void* GetWindowHandle() const = 0;

    // todo: GetType() and GraphicsSurfaceType; only the SDL3 GPU backend exists so far.

    // Initialises the graphics backend once the window has been created.
    virtual void Initialise() = 0;

    // The drawable size of the window, in pixels.
    [[nodiscard]] virtual glm::ivec2 GetDrawableSize() const = 0;
};

} // namespace panlid::platform
