#pragma once

#include "panlid/graphics/Easing.hh"
#include "panlid/graphics/transforms/EasingFunction.hh"
#include <cmath>
#include <numbers>

namespace panlid::graphics::transforms {

class DefaultEasingFunction {
  public:
    constexpr explicit DefaultEasingFunction(graphics::Easing easing) : easing_(easing) {}

    [[nodiscard]] double ApplyEasing(double time) const {
        switch (easing_) {
        default:
            return time;

        case graphics::Easing::In:
        case graphics::Easing::InQuad:
            return time * time;

        case graphics::Easing::Out:
        case graphics::Easing::OutQuad:
            return time * (2 - time);

        case graphics::Easing::InOutQuad:
            return applyInOutQuad(time);

        case graphics::Easing::InCubic:
            return time * time * time;

        case graphics::Easing::OutCubic:
            return applyOutCubic(time);

        case graphics::Easing::InOutCubic:
            return applyInOutCubic(time);

        case graphics::Easing::InQuart:
            return time * time * time * time;

        case graphics::Easing::OutQuart:
            return applyOutQuart(time);

        case graphics::Easing::InOutQuart:
            return applyInOutQuart(time);

        case graphics::Easing::InQuint:
            return time * time * time * time * time;

        case graphics::Easing::OutQuint:
            return applyOutQuint(time);

        case graphics::Easing::InOutQuint:
            return applyInOutQuint(time);

        case graphics::Easing::InSine:
            return 1 - std::cos(time * std::numbers::pi * .5);

        case graphics::Easing::OutSine:
            return std::sin(time * std::numbers::pi * .5);

        case graphics::Easing::InOutSine:
            return .5 - .5 * std::cos(std::numbers::pi * time);

        case graphics::Easing::InExpo:
            return applyInExpo(time);

        case graphics::Easing::OutExpo:
            return applyOutExpo(time);

        case graphics::Easing::InOutExpo:
            return applyInOutExpo(time);

        case graphics::Easing::InCirc:
            return 1 - std::sqrt(1 - time * time);

        case graphics::Easing::OutCirc:
            return applyOutCirc(time);

        case graphics::Easing::InOutCirc:
            return applyInOutCirc(time);

        case graphics::Easing::InElastic:
            return applyInElastic(time);

        case graphics::Easing::OutElastic:
            return applyOutElastic(time);

        case graphics::Easing::OutElasticHalf:
            return applyOutElasticHalf(time);

        case graphics::Easing::OutElasticQuarter:
            return applyOutElasticQuarter(time);

        case graphics::Easing::InOutElastic:
            return applyInOutElastic(time);

        case graphics::Easing::InBack:
            return applyInBack(time);

        case graphics::Easing::OutBack:
            return applyOutBack(time);

        case graphics::Easing::InOutBack:
            return applyInOutBack(time);

        case graphics::Easing::InBounce:
            return applyInBounce(time);

        case graphics::Easing::OutBounce:
            return applyOutBounce(time);

        case graphics::Easing::InOutBounce:
            return applyInOutBounce(time);

        case graphics::Easing::OutPow10:
            return applyOutPow10(time);
        }
    }

  private:
    static constexpr double kElasticConst = 2 * std::numbers::pi / .3;
    static constexpr double kElasticConst2 = .3 / 4;

    static constexpr double kBackConst = 1.70158;
    static constexpr double kBackConst2 = kBackConst * 1.525;

    static constexpr double kBounceConst = 1.0 / 2.75;

    static constexpr double kExpoOffset = 1.0 / 1024.0;
    static constexpr double kElasticOffsetFull = 1.0 / 2048.0;
    static const inline double ElasticOffsetHalf =
        std::pow(2, -10) * std::sin((.5 - kElasticConst2) * kElasticConst);
    static const inline double ElasticOffsetQuarter =
        std::pow(2, -10) * std::sin((.25 - kElasticConst2) * kElasticConst);
    static const inline double InOutElasticOffset =
        std::pow(2, -10) * std::sin((1 - kElasticConst2 * 1.5) * kElasticConst / 1.5);

    static double applyInOutQuad(double time) {
        if (time < .5) {
            return time * time * 2;
        }
        time -= 1;
        return time * time * -2 + 1;
    }

    static double applyOutCubic(double time) {
        time -= 1;
        return time * time * time + 1;
    }

    static double applyInOutCubic(double time) {
        if (time < .5) {
            return time * time * time * 4;
        }
        time -= 1;
        return time * time * time * 4 + 1;
    }

    static double applyOutQuart(double time) {
        time -= 1;
        return 1 - time * time * time * time;
    }

    static double applyInOutQuart(double time) {
        if (time < .5) {
            return time * time * time * time * 8;
        }
        time -= 1;
        return time * time * time * time * -8 + 1;
    }

    static double applyOutQuint(double time) {
        time -= 1;
        return time * time * time * time * time + 1;
    }

    static double applyInOutQuint(double time) {
        if (time < .5) {
            return time * time * time * time * time * 16;
        }
        time -= 1;
        return time * time * time * time * time * 16 + 1;
    }

    static double applyInExpo(double time) {
        return std::pow(2, 10 * (time - 1)) + kExpoOffset * (time - 1);
    }

    static double applyOutExpo(double time) {
        return -std::pow(2, -10 * time) + 1 + kExpoOffset * time;
    }

    static double applyInOutExpo(double time) {
        if (time < .5) {
            return .5 * (std::pow(2, 20 * time - 10) + kExpoOffset * (2 * time - 1));
        }
        return 1 - .5 * (std::pow(2, -20 * time + 10) + kExpoOffset * (-2 * time + 1));
    }

    static double applyOutCirc(double time) {
        time -= 1;
        return std::sqrt(1 - time * time);
    }

    static double applyInOutCirc(double time) {
        time *= 2;
        if (time < 1) {
            return .5 - .5 * std::sqrt(1 - time * time);
        }
        time -= 2;
        return .5 * std::sqrt(1 - time * time) + .5;
    }

    static double applyInElastic(double time) {
        return -std::pow(2, -10 + 10 * time) *
                   std::sin((1 - kElasticConst2 - time) * kElasticConst) +
               kElasticOffsetFull * (1 - time);
    }

    static double applyOutElastic(double time) {
        return std::pow(2, -10 * time) * std::sin((time - kElasticConst2) * kElasticConst) + 1 -
               kElasticOffsetFull * time;
    }

    static double applyOutElasticHalf(double time) {
        return std::pow(2, -10 * time) * std::sin((.5 * time - kElasticConst2) * kElasticConst) +
               1 - ElasticOffsetHalf * time;
    }

    static double applyOutElasticQuarter(double time) {
        return std::pow(2, -10 * time) * std::sin((.25 * time - kElasticConst2) * kElasticConst) +
               1 - ElasticOffsetQuarter * time;
    }

    static double applyInOutElastic(double time) {
        time *= 2;
        if (time < 1) {
            return -.5 * (std::pow(2, -10 + 10 * time) *
                              std::sin((1 - kElasticConst2 * 1.5 - time) * kElasticConst / 1.5) -
                          InOutElasticOffset * (1 - time));
        }
        time -= 1;
        return .5 * (std::pow(2, -10 * time) *
                         std::sin((time - kElasticConst2 * 1.5) * kElasticConst / 1.5) -
                     InOutElasticOffset * time) +
               1;
    }

    static double applyInBack(double time) {
        return time * time * ((kBackConst + 1) * time - kBackConst);
    }

    static double applyOutBack(double time) {
        time -= 1;
        return time * time * ((kBackConst + 1) * time + kBackConst) + 1;
    }

    static double applyInOutBack(double time) {
        time *= 2;
        if (time < 1) {
            return .5 * time * time * ((kBackConst2 + 1) * time - kBackConst2);
        }
        time -= 2;
        return .5 * (time * time * ((kBackConst2 + 1) * time + kBackConst2) + 2);
    }

    static double applyInBounce(double time) { return 1 - applyOutBounce(1 - time); }

    static double applyOutBounce(double time) {
        if (time < kBounceConst) {
            return 7.5625 * time * time;
        }
        if (time < 2 * kBounceConst) {
            time -= 1.5 * kBounceConst;
            return 7.5625 * time * time + .75;
        }
        if (time < 2.5 * kBounceConst) {
            time -= 2.25 * kBounceConst;
            return 7.5625 * time * time + .9375;
        }
        time -= 2.625 * kBounceConst;
        return 7.5625 * time * time + .984375;
    }

    static double applyInOutBounce(double time) {
        if (time < .5) {
            return .5 - .5 * applyOutBounce(1 - time * 2);
        }
        return applyOutBounce((time - .5) * 2) * .5 + .5;
    }

    static double applyOutPow10(double time) {
        time -= 1;
        return time * std::pow(time, 10) + 1;
    }

    Easing easing_;
};

static_assert(EasingFunction<DefaultEasingFunction>,
              "DefaultEasingFunction must satisfy the EasingFunction concept");

} // namespace panlid::graphics::transforms
