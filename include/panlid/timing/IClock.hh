#pragma once

namespace panlid::timing {

// All times are in milliseconds.
class IClock {
  public:
    virtual ~IClock() = default;

    [[nodiscard]] virtual double GetCurrentTime() const = 0;

    [[nodiscard]] virtual double GetRate() const = 0;

    [[nodiscard]] virtual bool IsRunning() const = 0;
};

} // namespace panlid::timing
