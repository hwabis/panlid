#pragma once

namespace panlid::graphics::rendering {

class IShaderPart {
  public:
    IShaderPart() = default;
    IShaderPart(const IShaderPart&) = delete;
    IShaderPart(IShaderPart&&) = delete;
    IShaderPart& operator=(const IShaderPart&) = delete;
    IShaderPart& operator=(IShaderPart&&) = delete;
    virtual ~IShaderPart() = default;
};

} // namespace panlid::graphics::rendering
