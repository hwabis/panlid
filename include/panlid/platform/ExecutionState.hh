#pragma once

#include <cstdint>

namespace panlid::platform {

// The game's execution states. A GameHost is in each state at most once. The order matters.
enum class ExecutionState : uint8_t {
    // Run has not been called yet.
    Idle = 0,

    // Execution has completely stopped.
    Stopped = 1,

    // Exit was requested, and the host is waiting to stop everything on the right thread.
    Stopping = 2,

    // Run has been called.
    Running = 3,
};

} // namespace panlid::platform
