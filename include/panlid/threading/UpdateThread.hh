#pragma once

#include "panlid/development/ThreadSafety.hh"
#include "panlid/platform/ExecutionMode.hh"
#include "panlid/threading/DrawThread.hh"
#include "panlid/threading/GameThread.hh"
#include <functional>
#include <memory>
#include <utility>

namespace panlid::threading {

class UpdateThread : public GameThread {
  public:
    UpdateThread(std::function<void()> onNewFrame, std::shared_ptr<DrawThread> drawThread)
        : GameThread{std::move(onNewFrame), "Update"}, drawThread_{std::move(drawThread)} {}

    [[nodiscard]] bool IsCurrent() const override {
        return development::ThreadSafety::IsUpdateThread;
    }

  protected:
    void OnInitialize() final {
        if (development::ThreadSafety::CurrentExecutionMode !=
            platform::ExecutionMode::SingleThread) {
            // The update thread needs the renderer's maximum texture size, which the draw thread
            // sets while initialising.
            if (drawThread_ != nullptr) {
                drawThread_->WaitUntilInitialized();
            }
        }
    }

    void MakeCurrent() final {
        GameThread::MakeCurrent();

        development::ThreadSafety::IsUpdateThread = true;
    }

    // todo: StatisticsCounters, once PerformanceMonitor exists.

  private:
    std::shared_ptr<DrawThread> drawThread_;
};

} // namespace panlid::threading
