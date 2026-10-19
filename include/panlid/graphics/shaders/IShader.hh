#pragma once

#include "panlid/graphics/rendering/IUniformBuffer.hh"
#include <string_view>

namespace panlid::graphics::shaders {

class IShader {
  public:
    IShader() = default;
    IShader(const IShader&) = delete;
    IShader(IShader&&) = delete;
    IShader& operator=(const IShader&) = delete;
    IShader& operator=(IShader&&) = delete;
    virtual ~IShader() = default;

    virtual void Bind() = 0;
    virtual void Unbind() = 0;

    [[nodiscard]] virtual bool IsLoaded() const = 0;
    [[nodiscard]] virtual bool IsBound() const = 0;

    virtual void BindUniformBlock(std::string_view blockName,
                                  rendering::IUniformBuffer& buffer) = 0;
};

} // namespace panlid::graphics::shaders
