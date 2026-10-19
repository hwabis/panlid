#pragma once

#include <cstdint>

namespace panlid::graphics::rendering {

// Why a vertex batch was flushed, for profiling.
enum class FlushBatchSource : std::uint8_t {
    BindBuffer,
    BindTexture,
    FinishFrame,
    SetBlend,
    SetBlendMask,
    SetDepthInfo,
    SetFrameBuffer,
    SetMasking,
    SetProjection,
    SetScissor,
    SetShader,
    SetStencilInfo,
    SetUniform,
    UnbindTexture,
    SetActiveBatch,
    StorageBufferOverflow,
};

} // namespace panlid::graphics::rendering
