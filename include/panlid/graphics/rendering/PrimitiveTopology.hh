#pragma once

#include <cstdint>

namespace panlid::graphics::rendering {

enum class PrimitiveTopology : std::uint8_t {
    Points,
    Lines,
    LineStrip,
    Triangles,
    TriangleStrip,
};

}
