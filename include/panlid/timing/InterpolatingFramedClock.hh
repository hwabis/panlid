#pragma once

#include "panlid/timing/FramedClock.hh"
#include "panlid/timing/IClock.hh"
#include "panlid/timing/IFrameBasedClock.hh"
#include "panlid/timing/ISourceChangeableClock.hh"
#include "panlid/timing/StopwatchClock.hh"
#include "panlid/utils/Interpolation.hh"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <memory>
#include <utility>

namespace panlid::timing {

class InterpolatingFramedClock final : public IFrameBasedClock, public ISourceChangeableClock {
  public:
    double AllowableErrorMilliseconds = 1000.0 / 60 * 2;
    double DriftRecoveryHalfLife = 50;

    explicit InterpolatingFramedClock(std::shared_ptr<IFrameBasedClock> source = nullptr) {
        InterpolatingFramedClock::ChangeSource(std::move(source));
        assert(source_ != nullptr);
        assert(framedSourceClock_ != nullptr);
    }

    [[nodiscard]] bool IsInterpolating() const { return isInterpolating_; }

    [[nodiscard]] double GetDrift() const {
        return GetCurrentTime() - framedSourceClock_->GetCurrentTime();
    }

    [[nodiscard]] double GetRate() const override { return framedSourceClock_->GetRate(); }

    [[nodiscard]] bool IsRunning() const override { return isRunning_; }

    [[nodiscard]] double GetElapsedFrameTime() const override { return elapsedFrameTime_; }

    [[nodiscard]] double GetFramesPerSecond() const override { return 0; }

    [[nodiscard]] double GetCurrentTime() const override { return currentTime_; }

    [[nodiscard]] const std::shared_ptr<IClock>& GetSource() const override { return source_; }

    void ChangeSource(std::shared_ptr<IClock> source) override {
        if (source != nullptr && source == source_) {
            return;
        }

        source_ = source != nullptr ? source : std::make_shared<StopwatchClock>(true);

        framedSourceClock_ = std::dynamic_pointer_cast<IFrameBasedClock>(source_);
        if (framedSourceClock_ == nullptr) {
            framedSourceClock_ = std::make_shared<FramedClock>(std::move(source));
        }

        isInterpolating_ = false;
        internalTime_ = framedSourceClock_->GetCurrentTime();
    }

    void ProcessFrame() override {
        const double lastTime = internalTime_;

        realtimeClock_.ProcessFrame();
        framedSourceClock_->ProcessFrame();

        const bool sourceIsRunning = framedSourceClock_->IsRunning();
        const bool sourceHasElapsed = framedSourceClock_->GetElapsedFrameTime() != 0;

        advanceTime(lastTime, sourceIsRunning, sourceHasElapsed);

        isRunning_ = sourceIsRunning;
        currentTime_ = internalTime_;
        elapsedFrameTime_ = internalTime_ - lastTime;
    }

  private:
    [[nodiscard]] static int sign(double value) {
        return static_cast<int>(value > 0) - static_cast<int>(value < 0);
    }

    void advanceTime(double lastTime, bool sourceIsRunning, bool sourceHasElapsed) {
        if (!sourceIsRunning) {
            if (sourceHasElapsed) {
                isInterpolating_ = false;
                internalTime_ = framedSourceClock_->GetCurrentTime();
            }

            return;
        }

        if (isInterpolating_) {
            internalTime_ += realtimeClock_.GetElapsedFrameTime() * GetRate();

            internalTime_ = utils::DampContinuously(
                internalTime_, framedSourceClock_->GetCurrentTime(), DriftRecoveryHalfLife,
                realtimeClock_.GetElapsedFrameTime());

            const bool withinAllowableError =
                std::abs(framedSourceClock_->GetCurrentTime() - internalTime_) <=
                AllowableErrorMilliseconds * GetRate();

            if (!withinAllowableError) {
                isInterpolating_ = false;
                internalTime_ = framedSourceClock_->GetCurrentTime();
            }
        } else {
            internalTime_ = framedSourceClock_->GetCurrentTime();

            if (sourceHasElapsed) {
                isInterpolating_ = true;
            }
        }

        const bool elapsedInOpposingDirection =
            framedSourceClock_->GetElapsedFrameTime() != 0 &&
            sign(framedSourceClock_->GetElapsedFrameTime()) != sign(GetRate());

        if (!elapsedInOpposingDirection) {
            internalTime_ = GetRate() >= 0 ? std::max(lastTime, internalTime_)
                                           : std::min(lastTime, internalTime_);
        }
    }

    std::shared_ptr<IClock> source_;
    std::shared_ptr<IFrameBasedClock> framedSourceClock_;
    FramedClock realtimeClock_{std::make_shared<StopwatchClock>(true)};

    bool isInterpolating_ = false;
    bool isRunning_ = false;
    double elapsedFrameTime_ = 0;
    double currentTime_ = 0;
    double internalTime_ = 0;
};

} // namespace panlid::timing
