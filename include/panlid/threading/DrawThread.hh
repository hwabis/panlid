#pragma once

#include "panlid/development/ThreadSafety.hh"
#include "panlid/threading/GameThread.hh"
#include <functional>
#include <utility>

namespace panlid::platform {

class GameHost;

} // namespace panlid::platform

namespace panlid::threading {

class DrawThread : public GameThread {
  public:
    DrawThread(std::function<void()> onNewFrame, platform::GameHost& host)
        : GameThread{std::move(onNewFrame), "Draw"}, host_{host} {}

    [[nodiscard]] bool IsCurrent() const override {
        return development::ThreadSafety::IsDrawThread;
    }

  protected:
    void OnInitialize() final;

    void MakeCurrent() final {
        GameThread::MakeCurrent();

        development::ThreadSafety::IsDrawThread = true;
    }

    void OnSuspended() final;

    // todo: StatisticsCounters, once PerformanceMonitor exists.

  private:
    platform::GameHost& host_;
};

} // namespace panlid::threading
