#pragma once

#include "panlid/logging/Logger.hh"
#include "panlid/platform/sdl3/Sdl3Exception.hh"
#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <format>
#include <source_location>
#include <string_view>

namespace panlid::platform::sdl3 {

namespace detail {

inline void LogError(const std::source_location& location) {
    logging::Logger::Log(std::format("SDL error: {}", SDL_GetError()));
    logging::Logger::Log(std::format("at {}:{}", location.file_name(), location.line()));
}

} // namespace detail

inline bool
LogErrorIfFailed(bool success,
                 const std::source_location& location = std::source_location::current()) {
    if (!success) {
        detail::LogError(location);
    }
    return success;
}

// For calls that return -1 on failure.
inline int
LogErrorIfFailed(int value,
                 const std::source_location& location = std::source_location::current()) {
    if (value == -1) {
        detail::LogError(location);
    }
    return value;
}

inline void ThrowIfFailed(bool success,
                          const std::source_location& location = std::source_location::current()) {
    if (!success) {
        throw Sdl3Exception{location};
    }
}

[[nodiscard]] inline std::string_view ReadableName(SDL_LogCategory category) {
    switch (category) {
    case SDL_LOG_CATEGORY_APPLICATION:
        return "application";
    case SDL_LOG_CATEGORY_ERROR:
        return "error";
    case SDL_LOG_CATEGORY_ASSERT:
        return "assert";
    case SDL_LOG_CATEGORY_SYSTEM:
        return "system";
    case SDL_LOG_CATEGORY_AUDIO:
        return "audio";
    case SDL_LOG_CATEGORY_VIDEO:
        return "video";
    case SDL_LOG_CATEGORY_RENDER:
        return "render";
    case SDL_LOG_CATEGORY_INPUT:
        return "input";
    case SDL_LOG_CATEGORY_TEST:
        return "test";
    default:
        return "unknown";
    }
}

[[nodiscard]] inline std::string_view ReadableName(SDL_LogPriority priority) {
    switch (priority) {
    case SDL_LOG_PRIORITY_VERBOSE:
        return "verbose";
    case SDL_LOG_PRIORITY_DEBUG:
        return "debug";
    case SDL_LOG_PRIORITY_INFO:
        return "info";
    case SDL_LOG_PRIORITY_WARN:
        return "warn";
    case SDL_LOG_PRIORITY_ERROR:
        return "error";
    case SDL_LOG_PRIORITY_CRITICAL:
        return "critical";
    default:
        return "unknown";
    }
}

// todo: the rest of this file, as the input, display and joystick code that needs it is ported:
// key and scancode mapping, window state and flags, joystick axis and button mapping, SDL rects,
// text input types, display modes, pen device types, ReadableString, GetAndClearError, and the
// error helpers for other return types (pointers, display IDs, strings, property IDs).

} // namespace panlid::platform::sdl3
