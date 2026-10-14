#pragma once

#include <cstdint>

namespace panlid::graphics {

enum class BlendingType : std::uint8_t {
    Inherit,
    ConstantAlpha,
    ConstantColor,
    DstAlpha,
    DstColor,
    One,
    OneMinusConstantAlpha,
    OneMinusConstantColor,
    OneMinusDstAlpha,
    OneMinusDstColor,
    OneMinusSrcAlpha,
    OneMinusSrcColor,
    SrcAlpha,
    SrcAlphaSaturate,
    SrcColor,
    Zero,
};

}
