#pragma once

#include "panlid/timing/IClock.hh"
#include "panlid/timing/IFrameBasedClock.hh"
#include "panlid/timing/ISourceChangeableClock.hh"
#include "panlid/timing/StopwatchClock.hh"
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>

namespace panlid::timing {

class FramedClock : public IFrameBasedClock, public ISourceChangeableClock {
  public:
    explicit FramedClock(std::shared_ptr<IClock> source = nullptr, bool processSource = true)
        : processSource_{processSource} {
        FramedClock::ChangeSource(source != nullptr ? std::move(source)
                                                    : std::make_shared<StopwatchClock>(true));
        assert(source_ != nullptr);
    }

    [[nodiscard]] const std::shared_ptr<IClock>& GetSource() const override { return source_; }

    [[nodiscard]] double GetFramesPerSecond() const override { return framesPerSecond_; }

    [[nodiscard]] double GetJitter() const { return jitter_; }

    [[nodiscard]] double GetCurrentTime() const override { return currentTime_; }

    [[nodiscard]] double GetRate() const override { return source_->GetRate(); }

    [[nodiscard]] double GetElapsedFrameTime() const override {
        return GetCurrentTime() - GetLastFrameTime();
    }

    [[nodiscard]] bool IsRunning() const override { return source_->IsRunning(); }

    void ChangeSource(std::shared_ptr<IClock> source) override {
        source_ = std::move(source);

        const double time = source_->GetCurrentTime();
        SetLastFrameTime(time);
        SetCurrentTime(time);
    }

    void ProcessFrame() override {
        betweenFrameTimes_[totalFramesProcessed_ % betweenFrameTimes_.size()] =
            GetCurrentTime() - GetLastFrameTime();
        ++totalFramesProcessed_;

        if (processSource_) {
            if (auto* framedSource = dynamic_cast<IFrameBasedClock*>(source_.get())) {
                framedSource->ProcessFrame();
            }
        }

        if (timeUntilNextCalculation_ <= 0) {
            timeUntilNextCalculation_ += kFpsCalculationInterval;

            calculateFramesPerSecondAndJitter();

            timeSinceLastCalculation_ = 0;
            framesSinceLastCalculation_ = 0;
        }

        ++framesSinceLastCalculation_;
        timeUntilNextCalculation_ -= GetElapsedFrameTime();
        timeSinceLastCalculation_ += GetElapsedFrameTime();

        SetLastFrameTime(GetCurrentTime());
        SetCurrentTime(GetSourceTime());
    }

  protected:
    virtual void SetCurrentTime(double value) { currentTime_ = value; }

    [[nodiscard]] virtual double GetLastFrameTime() const { return lastFrameTime_; }

    virtual void SetLastFrameTime(double value) { lastFrameTime_ = value; }

    [[nodiscard]] double GetSourceTime() const { return source_->GetCurrentTime(); }

  private:
    static constexpr double kFpsCalculationInterval = 250;

    void calculateFramesPerSecondAndJitter() {
        if (framesSinceLastCalculation_ == 0) {
            framesPerSecond_ = 0;
            jitter_ = 0;
            return;
        }

        framesPerSecond_ = std::ceil(static_cast<float>(framesSinceLastCalculation_) * 1000.0F /
                                     timeSinceLastCalculation_);

        double sum = 0;
        double sumOfSquares = 0;

        for (const double value : betweenFrameTimes_) {
            sum += value;
            sumOfSquares += value * value;
        }

        const auto count = static_cast<double>(betweenFrameTimes_.size());
        const double average = sum / count;
        const double variance = (sumOfSquares / count) - (average * average);
        jitter_ = std::sqrt(variance);
    }

    std::shared_ptr<IClock> source_;
    bool processSource_;

    std::array<double, 128> betweenFrameTimes_{};
    std::size_t totalFramesProcessed_ = 0;

    double framesPerSecond_ = 0;
    double jitter_ = 0;
    double currentTime_ = 0;
    double lastFrameTime_ = 0;

    double timeUntilNextCalculation_ = 0;
    double timeSinceLastCalculation_ = 0;
    int framesSinceLastCalculation_ = 0;
};

} // namespace panlid::timing
