#include "panlid/graphics/shaders/ShaderCompiler.hh"
#include <format>
#include <magic_enum/magic_enum.hpp>
#include <shaderc/shaderc.hpp>
#include <stdexcept>

namespace panlid::graphics::shaders {

namespace {

shaderc_shader_kind ToShadercKind(ShaderStage stage) {
    switch (stage) {
    case ShaderStage::Vertex:
        return shaderc_vertex_shader;
    case ShaderStage::Fragment:
        return shaderc_fragment_shader;
    }

    throw std::invalid_argument("Unknown shader stage.");
}

} // namespace

std::vector<std::uint32_t> CompileGlslToSpirv(std::string_view source, ShaderStage stage,
                                              const std::string& name) {
    // todo: reuse one compiler and the options instead of creating them per call
    // todo: set an optimisation level once shaders are real
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetSourceLanguage(shaderc_source_language_glsl);
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_0);

    const shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(
        source.data(), source.size(), ToShadercKind(stage), name.c_str(), options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        throw std::runtime_error(std::format("Failed to compile {} shader '{}': {}",
                                             magic_enum::enum_name(stage), name,
                                             result.GetErrorMessage()));
    }

    return {result.cbegin(), result.cend()};
}

} // namespace panlid::graphics::shaders
