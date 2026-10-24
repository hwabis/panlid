#pragma once

#include <cstddef>
#include <span>

namespace panlid::graphics::rendering {

// A buffer which stores data for a uniform block.
// todo: IShaderStorageBufferObject (indexed element access) once something needs it.
class IUniformBuffer {
  public:
    IUniformBuffer() = default;
    IUniformBuffer(const IUniformBuffer&) = delete;
    IUniformBuffer(IUniformBuffer&&) = delete;
    IUniformBuffer& operator=(const IUniformBuffer&) = delete;
    IUniformBuffer& operator=(IUniformBuffer&&) = delete;
    virtual ~IUniformBuffer() = default;

    // Replaces the buffer's contents. The data must be exactly as large as the buffer.
    virtual void SetData(std::span<const std::byte> data) = 0;
};

} // namespace panlid::graphics::rendering
