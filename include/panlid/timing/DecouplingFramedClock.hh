#pragma once

#include "panlid/timing/IAdjustableClock.hh"
#include "panlid/timing/IClock.hh"
#include "panlid/timing/IFrameBasedClock.hh"
#include "panlid/timing/ISourceChangeableClock.hh"
#include "panlid/timing/StopwatchClock.hh"
#include <cassert>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

namespace panlid::timing {

class DecouplingFramedClock final : public ISourceChangeableClock,
                                    public IAdjustableClock,
                                    public IFrameBasedClock {
  public:
    bool AllowDecoupling = true;

    explicit DecouplingFramedClock(std::shared_ptr<IClock> source = nullptr) {
        DecouplingFramedClock::ChangeSource(std::move(source));
        assert(source_ != nullptr);
        assert(adjustableSourceClock_ != nullptr);
    }

    [[nodiscard]] bool IsRunning() const override { return isRunning_; }

    [[nodiscard]] double GetCurrentTime() const override { return currentTime_; }

    [[nodiscard]] double GetElapsedFrameTime() const override { return elapsedFrameTime_; }

    [[nodiscard]] double GetFramesPerSecond() const override { return 0; }

    [[nodiscard]] double GetRate() const override { return adjustableSourceClock_->GetRate(); }

    [[nodiscard]] const std::shared_ptr<IClock>& GetSource() const override { return source_; }

    void ChangeSource(std::shared_ptr<IClock> source) override {
        if (source == nullptr) {
            source = std::make_shared<StopwatchClock>(true);
        }

        auto* adjustableSource = dynamic_cast<IAdjustableClock*>(source.get());
        if (adjustableSource == nullptr) {
            throw std::invalid_argument("Clock must be of type IAdjustableClock");
        }

        source_ = std::move(source);
        adjustableSourceClock_ = adjustableSource;
        internalTime_ = adjustableSource->GetCurrentTime();
        shouldBeRunning_ = adjustableSource->IsRunning();
        lastSeekFailed_ = false;
    }

    void ProcessFrame() override {
        const double lastTime = currentTime_;

        if (auto* framedSource = dynamic_cast<IFrameBasedClock*>(source_.get())) {
            framedSource->ProcessFrame();
        }

        const double referenceTime = realtimeReferenceClock_.GetCurrentTime();

        advanceTime(referenceTime);

        isRunning_ = shouldBeRunning_;
        lastReferenceTime_ = referenceTime;
        currentTime_ = internalTime_;
        elapsedFrameTime_ = currentTime_ - lastTime;
    }

    void Reset() override {
        adjustableSourceClock_->Reset();
        pendingSourceRestartAfterNegativeSeek_ = false;
        shouldBeRunning_ = false;
        lastSeekFailed_ = false;
        internalTime_ = 0;
    }

    void Start() override {
        if (shouldBeRunning_) {
            return;
        }

        if (lastSeekFailed_ && AllowDecoupling) {
            shouldBeRunning_ = true;
            return;
        }

        adjustableSourceClock_->Start();
        shouldBeRunning_ = adjustableSourceClock_->IsRunning() || AllowDecoupling;
    }

    void Stop() override {
        adjustableSourceClock_->Stop();
        shouldBeRunning_ = false;
    }

    bool Seek(double position) override {
        lastSeekFailed_ = !adjustableSourceClock_->Seek(position);

        if (!lastSeekFailed_) {
            if (shouldBeRunning_ && !source_->IsRunning()) {
                adjustableSourceClock_->Start();
            }
        } else {
            if (!AllowDecoupling) {
                return false;
            }

            adjustableSourceClock_->Stop();
            pendingSourceRestartAfterNegativeSeek_ = position < 0;
        }

        internalTime_ = position;
        return true;
    }

    void SetRate(double rate) override { adjustableSourceClock_->SetRate(rate); }

    void ResetSpeedAdjustments() override { adjustableSourceClock_->ResetSpeedAdjustments(); }

  private:
    void advanceTime(double referenceTime) {
        if (source_->IsRunning()) {
            internalTime_ = source_->GetCurrentTime();
            shouldBeRunning_ = true;
            return;
        }

        if (!AllowDecoupling) {
            internalTime_ = source_->GetCurrentTime();
            shouldBeRunning_ = false;
            return;
        }

        if (!shouldBeRunning_) {
            return;
        }

        if (!lastReferenceTime_.has_value()) {
            return;
        }

        const double elapsedReferenceTime = (referenceTime - *lastReferenceTime_) * GetRate();

        internalTime_ += elapsedReferenceTime;

        if (pendingSourceRestartAfterNegativeSeek_ && internalTime_ >= 0) {
            pendingSourceRestartAfterNegativeSeek_ = false;

            lastSeekFailed_ = !adjustableSourceClock_->Seek(internalTime_);
            if (!lastSeekFailed_) {
                adjustableSourceClock_->Start();
            }
        }
    }

    std::shared_ptr<IClock> source_;
    IAdjustableClock* adjustableSourceClock_ = nullptr;
    StopwatchClock realtimeReferenceClock_{true};

    bool isRunning_ = false;
    double currentTime_ = 0;
    double elapsedFrameTime_ = 0;

    bool shouldBeRunning_ = false;
    double internalTime_ = 0;
    std::optional<double> lastReferenceTime_;
    bool lastSeekFailed_ = false;
    bool pendingSourceRestartAfterNegativeSeek_ = false;
};

} // namespace panlid::timing
