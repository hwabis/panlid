#pragma once

#include <cstdint>

namespace panlid::platform {

// todo port the [Description] strings ("Single thread", "Multithreaded") when the settings
// UI needs them
enum class ExecutionMode : uint8_t {
    SingleThread,
    MultiThreaded,
};

} // namespace panlid::platform
