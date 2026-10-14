#pragma once

#include <cstdint>

namespace panlid::graphics::rendering {

enum class StencilOperation : std::uint8_t {
    Zero,
    Invert,
    Keep,
    Replace,
    Increase,
    Decrease,
    IncreaseWrap,
    DecreaseWrap,
};

}
