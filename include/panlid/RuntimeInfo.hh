#pragma once

#include <cstdint>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

namespace panlid {

// Describes the platform the program was compiled for.
class RuntimeInfo {
  public:
    enum class Platform : uint8_t {
        Windows = 1,
        Linux = 2,
        MacOS = 3,
        IOS = 4,
        Android = 5,
    };

    static constexpr Platform kOS =
#if defined(_WIN32)
        Platform::Windows;
#elif defined(__ANDROID__)
        Platform::Android;
#elif defined(__APPLE__) && TARGET_OS_IPHONE
        Platform::IOS;
#elif defined(__APPLE__)
        Platform::MacOS;
#elif defined(__linux__)
        Platform::Linux;
#else
#error "Unsupported operating system."
#endif

    static constexpr bool kIsUnix = kOS != Platform::Windows;
    static constexpr bool kIsDesktop =
        kOS == Platform::Linux || kOS == Platform::MacOS || kOS == Platform::Windows;
    static constexpr bool kIsMobile = kOS == Platform::IOS || kOS == Platform::Android;
    static constexpr bool kIsApple = kOS == Platform::IOS || kOS == Platform::MacOS;

    // todo: StartupDirectory, once something needs the executable's directory.

    RuntimeInfo() = delete;
};

} // namespace panlid
