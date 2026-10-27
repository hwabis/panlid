#include "panlid/graphics/shaders/ShaderCompiler.hh"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <stdexcept>

namespace {

constexpr std::uint32_t kSpirvMagic = 0x07230203;

constexpr auto kVertexSource = R"(#version 450
layout(location = 0) in vec2 aPosition;
void main() { gl_Position = vec4(aPosition, 0.0, 1.0); }
)";

constexpr auto kFragmentSource = R"(#version 450
layout(location = 0) out vec4 oColour;
void main() { oColour = vec4(1.0); }
)";

} // namespace

using panlid::graphics::shaders::CompileGlslToSpirv;
using panlid::graphics::shaders::ShaderStage;

TEST_CASE("ReturnsSpirvForVertexShader", "[ShaderCompiler]") {
    const auto words = CompileGlslToSpirv(kVertexSource, ShaderStage::Vertex, "test.vs");

    REQUIRE_FALSE(words.empty());
    REQUIRE(words.front() == kSpirvMagic);
}

TEST_CASE("ReturnsSpirvForFragmentShader", "[ShaderCompiler]") {
    const auto words = CompileGlslToSpirv(kFragmentSource, ShaderStage::Fragment, "test.fs");

    REQUIRE_FALSE(words.empty());
    REQUIRE(words.front() == kSpirvMagic);
}

TEST_CASE("ThrowsOnInvalidSource", "[ShaderCompiler]") {
    REQUIRE_THROWS_AS(CompileGlslToSpirv("not glsl", ShaderStage::Fragment, "bad.fs"),
                      std::runtime_error);
}
