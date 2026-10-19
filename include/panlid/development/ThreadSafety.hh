#pragma once

#include "panlid/platform/ExecutionMode.hh"
#include <atomic>
#include <cassert>

namespace panlid::development {

class ThreadSafety {
  public:
    static inline thread_local bool IsInputThread = false;
    static inline thread_local bool IsUpdateThread = false;
    static inline thread_local bool IsDrawThread = false;
    static inline thread_local bool IsAudioThread = false;

    static inline std::atomic<platform::ExecutionMode> CurrentExecutionMode =
        platform::ExecutionMode::SingleThread;

    static void EnsureInputThread() { assert(IsInputThread); }

    static void EnsureUpdateThread() { assert(IsUpdateThread); }

    static void EnsureNotUpdateThread() { assert(!IsUpdateThread); }

    static void EnsureDrawThread() { assert(IsDrawThread); }

    static void EnsureAudioThread() { assert(IsAudioThread); }

    static void ResetAllForCurrentThread() {
        IsInputThread = false;
        IsUpdateThread = false;
        IsDrawThread = false;
        IsAudioThread = false;
    }
};

} // namespace panlid::development
