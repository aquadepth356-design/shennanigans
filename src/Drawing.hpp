#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include <glm/glm.hpp>
#include <imgui.h>
#include "Skeleton.hpp"
#include "Math.hpp"

// Convert glm::vec2 to ImVec2
inline ImVec2 ToImVec2(const glm::vec2& v) { return { v.x, v.y }; }

// ─── Full skeleton overlay ────────────────────────────────────────────────────

inline void DrawStickFigureOverlay(
    const std::vector<BoneVertex>&              bones,
    const std::unordered_map<std::string, int>& boneIndex,
    ImU32 lineColor   = IM_COL32(0,   255, 0,   255),  // green lines
    ImU32 jointColor  = IM_COL32(255, 255, 0,   255),  // yellow dots
    float lineThick   = 2.0f,
    float jointRadius = 4.0f)
{
    ImDrawList* dl = ImGui::GetBackgroundDrawList();

    // Draw bone connection lines
    for (const auto& [parentName, childName] : SKELETON_EDGES) {
        auto itP = boneIndex.find(parentName);
        auto itC = boneIndex.find(childName);
        if (itP == boneIndex.end() || itC == boneIndex.end()) continue;

        const auto& b0 = bones[itP->second];
        const auto& b1 = bones[itC->second];

        // Skip if either bone is behind the camera
        if (!b0.screenPos.has_value() || !b1.screenPos.has_value()) continue;

        dl->AddLine(
            ToImVec2(*b0.screenPos),
            ToImVec2(*b1.screenPos),
            lineColor, lineThick
        );
    }

    // Draw joint dots on top of lines
    for (const auto& bone : bones) {
        if (!bone.screenPos.has_value()) continue;
        dl->AddCircleFilled(ToImVec2(*bone.screenPos), jointRadius, jointColor);
    }
}

// ─── Upper body quick draw ────────────────────────────────────────────────────

inline void DrawUpperBodyOverlay(
    glm::vec3 head, glm::vec3 neck,
    glm::vec3 leftShoulder, glm::vec3 rightShoulder,
    const glm::mat4& viewProj,
    float W, float H,
    ImU32 color = IM_COL32(255, 255, 255, 255))
{
    auto toScreen = [&](glm::vec3 p) { return WorldToScreen(p, viewProj, W, H); };

    auto sHead  = toScreen(head);
    auto sNeck  = toScreen(neck);
    auto sLeft  = toScreen(leftShoulder);
    auto sRight = toScreen(rightShoulder);

    if (!sHead || !sNeck || !sLeft || !sRight) return;

    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    dl->AddLine(ToImVec2(*sHead),  ToImVec2(*sNeck),  color, 2.0f);
    dl->AddLine(ToImVec2(*sNeck),  ToImVec2(*sLeft),  color, 2.0f);
    dl->AddLine(ToImVec2(*sNeck),  ToImVec2(*sRight), color, 2.0f);
    dl->AddLine(ToImVec2(*sLeft),  ToImVec2(*sRight), color, 2.0f);
}

// ─── Simple 2D bounding box around a skeleton ────────────────────────────────

inline void DrawSkeletonBoundingBox(
    const std::vector<BoneVertex>& bones,
    ImU32 color = IM_COL32(255, 50, 50, 200),
    float thick = 1.5f)
{
    float minX =  1e9f, minY =  1e9f;
    float maxX = -1e9f, maxY = -1e9f;
    bool  any  = false;

    for (const auto& b : bones) {
        if (!b.screenPos.has_value()) continue;
        minX = std::min(minX, b.screenPos->x);
        minY = std::min(minY, b.screenPos->y);
        maxX = std::max(maxX, b.screenPos->x);
        maxY = std::max(maxY, b.screenPos->y);
        any  = true;
    }

    if (!any) return;

    // Add a small padding
    constexpr float pad = 6.0f;
    ImGui::GetBackgroundDrawList()->AddRect(
        { minX - pad, minY - pad },
        { maxX + pad, maxY + pad },
        color, 0.0f, 0, thick
    );
}
