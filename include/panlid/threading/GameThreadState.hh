#pragma once

#include <cstdint>

namespace panlid::threading {

enum class GameThreadState : uint8_t {
    NotStarted,
    Starting,
    Running,
    Paused,
    Exited,
};

} // namespace panlid::threading
