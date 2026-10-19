#pragma once

#include "panlid/development/ThreadSafety.hh"
#include "panlid/threading/GameThread.hh"

namespace panlid::threading {

class InputThread : public GameThread {
  public:
    InputThread() : GameThread{nullptr, "Input"} {}

    // todo port StatisticsCounters (mouse, key, joystick, midi, tablet, touch events)

    [[nodiscard]] bool IsCurrent() const override {
        return development::ThreadSafety::IsInputThread;
    }

  protected:
    void PrepareForWork() override { Initialize(true); }

    void MakeCurrent() final {
        GameThread::MakeCurrent();

        development::ThreadSafety::IsInputThread = true;
    }
};

} // namespace panlid::threading
