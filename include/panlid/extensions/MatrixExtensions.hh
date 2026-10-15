#pragma once

#include <cmath>
#include <glm/glm.hpp>

namespace panlid::extensions {

inline void TranslateFromLeft(glm::mat3& m, const glm::vec2& v) { m[2] += m[0] * v.x + m[1] * v.y; }

inline void TranslateFromRight(glm::mat3& m, const glm::vec2& v) {
    m[0][0] += m[0][2] * v.x;
    m[1][0] += m[1][2] * v.x;
    m[2][0] += m[2][2] * v.x;

    m[0][1] += m[0][2] * v.y;
    m[1][1] += m[1][2] * v.y;
    m[2][1] += m[2][2] * v.y;
}

inline void RotateFromLeft(glm::mat3& m, float radians) {
    const float cos = std::cos(radians);
    const float sin = std::sin(radians);

    const glm::vec3 row0 = m[0] * cos + m[1] * sin;
    m[1] = m[1] * cos - m[0] * sin;
    m[0] = row0;
}

inline void RotateFromRight(glm::mat3& m, float radians) {
    const float cos = std::cos(radians);
    const float sin = std::sin(radians);

    const float m11 = m[0][0] * cos - m[0][1] * sin;
    const float m21 = m[1][0] * cos - m[1][1] * sin;
    const float m31 = m[2][0] * cos - m[2][1] * sin;

    m[0][1] = m[0][1] * cos + m[0][0] * sin;
    m[1][1] = m[1][1] * cos + m[1][0] * sin;
    m[2][1] = m[2][1] * cos + m[2][0] * sin;

    m[0][0] = m11;
    m[1][0] = m21;
    m[2][0] = m31;
}

inline void ScaleFromLeft(glm::mat3& m, const glm::vec2& v) {
    m[0] *= v.x;
    m[1] *= v.y;
}

inline void ScaleFromRight(glm::mat3& m, const glm::vec2& v) {
    m[0][0] *= v.x;
    m[1][0] *= v.x;
    m[2][0] *= v.x;

    m[0][1] *= v.y;
    m[1][1] *= v.y;
    m[2][1] *= v.y;
}

// Shears in X first, then in Y.
inline void ShearFromLeft(glm::mat3& m, const glm::vec2& v) {
    const glm::vec3 row0 = m[0] + m[1] * v.y + m[0] * v.x * v.y;
    m[1] += m[0] * v.x;
    m[0] = row0;
}

// Shears in Y first, then in X.
inline void ShearFromRight(glm::mat3& m, const glm::vec2& v) {
    const float xy = v.x * v.y;

    const float m11 = m[0][0] + m[0][1] * v.x;
    const float m21 = m[1][0] + m[1][1] * v.x;
    const float m31 = m[2][0] + m[2][1] * v.x;

    m[0][1] += m[0][0] * v.y + m[0][1] * xy;
    m[1][1] += m[1][0] * v.y + m[1][1] * xy;
    m[2][1] += m[2][0] * v.y + m[2][1] * xy;

    m[0][0] = m11;
    m[1][0] = m21;
    m[2][0] = m31;
}

// Becomes the zero matrix if singular.
inline void FastInvert(glm::mat3& value) {
    const float d11 = value[1][1] * value[2][2] + value[1][2] * -value[2][1];
    const float d12 = value[1][0] * value[2][2] + value[1][2] * -value[2][0];
    const float d13 = value[1][0] * value[2][1] + value[1][1] * -value[2][0];

    float det = value[0][0] * d11 - value[0][1] * d12 + value[0][2] * d13;

    if (det == 0.F) {
        value = glm::mat3{0.F};
        return;
    }

    det = 1.F / det;

    const float d21 = value[0][1] * value[2][2] + value[0][2] * -value[2][1];
    const float d22 = value[0][0] * value[2][2] + value[0][2] * -value[2][0];
    const float d23 = value[0][0] * value[2][1] + value[0][1] * -value[2][0];

    const float d31 = value[0][1] * value[1][2] - value[0][2] * value[1][1];
    const float d32 = value[0][0] * value[1][2] - value[0][2] * value[1][0];
    const float d33 = value[0][0] * value[1][1] - value[0][1] * value[1][0];

    value[0][0] = +d11 * det;
    value[0][1] = -d21 * det;
    value[0][2] = +d31 * det;
    value[1][0] = -d12 * det;
    value[1][1] = +d22 * det;
    value[1][2] = -d32 * det;
    value[2][0] = +d13 * det;
    value[2][1] = -d23 * det;
    value[2][2] = +d33 * det;
}

} // namespace panlid::extensions
