#pragma once
#include <optional>
#include <glm/glm.hpp>

// Projects a 3D world-space position to 2D screen-space pixel coordinates.
// Returns nullopt if the point is behind the camera (clip.w <= 0).
inline std::optional<glm::vec2> WorldToScreen(
    const glm::vec3& worldPos,
    const glm::mat4& viewProj,
    float screenW,
    float screenH)
{
    glm::vec4 clip = viewProj * glm::vec4(worldPos, 1.0f);

    if (clip.w < 1e-5f)
        return std::nullopt; // behind the near plane

    glm::vec3 ndc = glm::vec3(clip) / clip.w; // perspective divide -> [-1, 1]

    return glm::vec2{
        (ndc.x * 0.5f + 0.5f) * screenW,
        (1.0f - (ndc.y * 0.5f + 0.5f)) * screenH  // Y-flip: NDC +Y = up, screen +Y = down
    };
}
