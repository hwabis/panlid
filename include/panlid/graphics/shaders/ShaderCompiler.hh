#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace panlid::graphics::shaders {

enum class ShaderStage : std::uint8_t { Vertex, Fragment };

// Throws std::runtime_error with the compiler's message if the source does not compile.
[[nodiscard]] std::vector<std::uint32_t>
CompileGlslToSpirv(std::string_view source, ShaderStage stage, const std::string& name);

} // namespace panlid::graphics::shaders

