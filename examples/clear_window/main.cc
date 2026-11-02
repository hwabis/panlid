#include "panlid/graphics/rendering/PrimitiveTopology.hh"
#include "panlid/graphics/rendering/Renderer.hh"
#include "panlid/graphics/rendering/sdl3/Sdl3GpuRenderer.hh"
#include "panlid/graphics/rendering/sdl3/Sdl3GpuShader.hh"
#include "panlid/graphics/rendering/sdl3/Sdl3GpuVertexBatch.hh"
#include "panlid/graphics/rendering/vertices/Vertex.hh"
#include "panlid/graphics/rendering/vertices/VertexMember.hh"
#include "panlid/graphics/shaders/ShaderCompiler.hh"
#include "panlid/logging/Logger.hh"
#include "panlid/platform/GameHost.hh"
#include "panlid/platform/IWindow.hh"
#include "panlid/platform/sdl3/Sdl3Window.hh"
#include <array>
#include <cstddef>
#include <exception>
#include <format>
#include <functional>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <iostream>
#include <memory>
#include <span>
#include <thread>

namespace {

using panlid::graphics::rendering::Renderer;
using panlid::graphics::rendering::sdl3::Sdl3GpuRenderer;
using panlid::logging::Logger;
using panlid::platform::GameHost;
using panlid::platform::IWindow;
using panlid::platform::sdl3::Sdl3Window;

// A number that is the same for the same thread and different for different threads.
[[nodiscard]] std::size_t CurrentThreadNumber() {
    return std::hash<std::thread::id>{}(std::this_thread::get_id());
}

struct TriangleVertex {
    glm::vec2 Position{};
    glm::vec3 Colour{};

    bool operator==(const TriangleVertex&) const = default;

    static std::span<const panlid::graphics::rendering::vertices::VertexMember> GetLayout() {
        using namespace panlid::graphics::rendering::vertices;

        static constexpr std::array kLayout{
            VertexMember{.Count = 2,
                         .Type = VertexComponentType::Float,
                         .Normalized = false,
                         .Offset = offsetof(TriangleVertex, Position)},
            VertexMember{.Count = 3,
                         .Type = VertexComponentType::Float,
                         .Normalized = false,
                         .Offset = offsetof(TriangleVertex, Colour)}};

        return kLayout;
    }
};

static_assert(panlid::graphics::rendering::vertices::Vertex<TriangleVertex>);

constexpr const char* kVertexSource = R"(#version 450
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec3 aColour;
layout(location = 0) out vec3 vColour;
void main() {
    vColour = aColour;
    gl_Position = vec4(aPosition, 0.0, 1.0);
}
)";

constexpr const char* kFragmentSource = R"(#version 450
layout(location = 0) in vec3 vColour;
layout(location = 0) out vec4 outColour;
void main() {
    outColour = vec4(vColour, 1.0);
}
)";

// Sdl3Window has a protected constructor, so the demo needs a concrete window type.
class DemoWindow final : public Sdl3Window {
  public:
    DemoWindow() : Sdl3Window("panlid clear_window") {}
};

class DemoHost final : public GameHost {
  public:
    DemoHost() : GameHost{"clear_window"} {}

  protected:
    std::unique_ptr<IWindow> CreateWindow() override { return std::make_unique<DemoWindow>(); }

    std::unique_ptr<Renderer> CreateRenderer() override {
        return std::make_unique<Sdl3GpuRenderer>();
    }

    void DrawScene() override {
        using namespace panlid::graphics;
        using namespace panlid::graphics::rendering;

        auto& renderer = static_cast<sdl3::Sdl3GpuRenderer&>(*GetRenderer());

        if (shader_ == nullptr) {
            shader_ = std::make_unique<sdl3::Sdl3GpuShader>(
                renderer, renderer.GetDevice(), "triangle",
                sdl3::Sdl3GpuShader::StageSource{
                    .Spirv = shaders::CompileGlslToSpirv(
                        kVertexSource, shaders::ShaderStage::Vertex, "triangle.vert")},
                sdl3::Sdl3GpuShader::StageSource{
                    .Spirv = shaders::CompileGlslToSpirv(
                        kFragmentSource, shaders::ShaderStage::Fragment, "triangle.frag")});

            batch_ = std::make_unique<sdl3::Sdl3GpuVertexBatch>(
                renderer, TriangleVertex::GetLayout(), sizeof(TriangleVertex), 3,
                PrimitiveTopology::Triangles);
        }

        shader_->Bind();

        const std::array vertices{
            TriangleVertex{.Position = {0.0F, 0.5F}, .Colour = {1.0F, 0.0F, 0.0F}},
            TriangleVertex{.Position = {0.5F, -0.5F}, .Colour = {0.0F, 1.0F, 0.0F}},
            TriangleVertex{.Position = {-0.5F, -0.5F}, .Colour = {0.0F, 0.0F, 1.0F}}};

        for (const auto& vertex : vertices) {
            batch_->AddVertex(std::as_bytes(std::span{&vertex, 1}));
        }

        batch_->Draw();
        batch_->ResetCounters();

        shader_->Unbind();
    }

    void ReleaseScene() override {
        batch_.reset();
        shader_.reset();
    }

  private:
    std::unique_ptr<panlid::graphics::rendering::sdl3::Sdl3GpuShader> shader_;
    std::unique_ptr<panlid::graphics::rendering::sdl3::Sdl3GpuVertexBatch> batch_;
    bool loggedUpdate_ = false;
    bool loggedDraw_ = false;
};

} // namespace

int main() {
    try {
        DemoHost host;

        // With no game to ask the host to exit, the demo does it.
        host.AddExitRequestedHandler([&host] { host.Exit(); });

        host.Run();
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
