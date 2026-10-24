#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <format>
#include <source_location>
#include <stdexcept>

namespace panlid::platform::sdl3 {

// Thrown when an SDL call fails. The message is SDL's last error and the call site.
class Sdl3Exception final : public std::runtime_error {
  public:
    explicit Sdl3Exception(const std::source_location& location = std::source_location::current())
        : std::runtime_error{std::format("{} (at {}:{})", SDL_GetError(), location.file_name(),
                                         location.line())} {}
};

} // namespace panlid::platform::sdl3
