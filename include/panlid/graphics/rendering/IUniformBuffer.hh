#pragma once

namespace panlid::graphics::rendering {

// A buffer which stores data for a uniform block.
// todo: port IUniformBuffer<TData> (typed Data property) once the renderer's buffer creation is
// designed.
class IUniformBuffer {
  public:
    IUniformBuffer() = default;
    IUniformBuffer(const IUniformBuffer&) = delete;
    IUniformBuffer(IUniformBuffer&&) = delete;
    IUniformBuffer& operator=(const IUniformBuffer&) = delete;
    IUniformBuffer& operator=(IUniformBuffer&&) = delete;
    virtual ~IUniformBuffer() = default;
};

} // namespace panlid::graphics::rendering
