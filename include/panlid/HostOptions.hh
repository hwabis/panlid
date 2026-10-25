#pragma once

#include <optional>
#include <string>

namespace panlid {

// Various configuration properties for a GameHost.
struct HostOptions {
    // The IPC pipe name to bind. Shared by all instances of an app that want to talk to each other.
    std::optional<std::string> IpcPipeName{};

    // Whether game files go next to the executable instead of the standard data directory.
    bool PortableInstallation{};

    // Whether to bypass the compositor. On Linux the compositor re-buffers the application, which
    // adds latency, so games usually want to bypass it. Has no effect if the
    // SDL_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR environment variable is set.
    bool BypassCompositor = true;

    // The name of the game shown to the user, such as in the window title bar. If empty, GameHost
    // chooses a default from the game name.
    std::string FriendlyGameName{};
};

} // namespace panlid
