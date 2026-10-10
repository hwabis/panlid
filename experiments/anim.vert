#version 450

layout(location = 0) out vec3 vColor;

// SDL_GPU (Vulkan/SPIR-V): vertex uniform buffers are set 1.
layout(set = 1, binding = 0) uniform Params {
    float angle;
    float aspect;
    vec2 padding;
} u;

const vec2 kPositions[3] = vec2[](vec2(0.0, 0.6), vec2(-0.52, -0.3), vec2(0.52, -0.3));
const vec3 kColors[3] = vec3[](vec3(1.0, 0.2, 0.2), vec3(0.2, 1.0, 0.2), vec3(0.3, 0.4, 1.0));

void main() {
    vec2 p = kPositions[gl_VertexIndex];
    float c = cos(u.angle);
    float s = sin(u.angle);
    vec2 r = vec2(c * p.x - s * p.y, s * p.x + c * p.y);
    r.x /= u.aspect;
    gl_Position = vec4(r, 0.0, 1.0);
    vColor = kColors[gl_VertexIndex];
}
