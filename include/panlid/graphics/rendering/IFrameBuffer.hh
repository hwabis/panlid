#pragma once

#include <glm/vec2.hpp>

namespace panlid::graphics::rendering {

// A render target backed by a texture.
class IFrameBuffer {
  public:
    IFrameBuffer() = default;
    IFrameBuffer(const IFrameBuffer&) = delete;
    IFrameBuffer(IFrameBuffer&&) = delete;
    IFrameBuffer& operator=(const IFrameBuffer&) = delete;
    IFrameBuffer& operator=(IFrameBuffer&&) = delete;
    virtual ~IFrameBuffer() = default;

    // todo: GetTexture(), the backing texture, once Texture is ported.

    [[nodiscard]] virtual glm::vec2 GetSize() const = 0;
    virtual void SetSize(glm::vec2 size) = 0;

    // Does not clear the buffer or reset the viewport and projection.
    virtual void Bind() = 0;

    virtual void Unbind() = 0;
};

} // namespace panlid::graphics::rendering
