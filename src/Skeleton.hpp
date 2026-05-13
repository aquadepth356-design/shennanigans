#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <glm/glm.hpp>
#include "Math.hpp"

struct BoneVertex {
    std::string              name;
    int                      index = -1;
    glm::vec3                worldPos = glm::vec3(0.f);
    std::optional<glm::vec2> screenPos;
};

inline std::vector<BoneVertex> ExtractBonePositions(
    const std::vector<glm::mat4>&   boneMatrices,
    const std::vector<std::string>& boneNames)
{
    std::vector<BoneVertex> verts;
    verts.reserve(boneMatrices.size());
    for (size_t i = 0; i < boneMatrices.size() && i < boneNames.size(); ++i) {
        BoneVertex v;
        v.name     = boneNames[i];
        v.index    = (int)i;
        // Position is stored in the translation column: mat[3] = (x, y, z, 1)
        v.worldPos = glm::vec3(boneMatrices[i][3]);
        verts.push_back(v);
    }
    return verts;
}

inline std::vector<BoneVertex> ExtractAllBonePositions(
    const std::vector<glm::mat4>& boneMatrices)
{
    std::vector<BoneVertex> verts;
    verts.reserve(boneMatrices.size());
    for (size_t i = 0; i < boneMatrices.size(); ++i) {
        BoneVertex v;
        v.name     = std::to_string(i);
        v.index    = (int)i;
        // Position is stored in the translation column: mat[3] = (x, y, z, 1)
        v.worldPos = glm::vec3(boneMatrices[i][3]);
        verts.push_back(v);
    }
    return verts;
}

inline void ProjectBones(
    std::vector<BoneVertex>& bones,
    const glm::mat4& viewProj,
    float screenW, float screenH)
{
    for (auto& bone : bones)
        bone.screenPos = WorldToScreen(bone.worldPos, viewProj, screenW, screenH);
}

inline std::unordered_map<std::string, int> BuildBoneIndex(
    const std::vector<std::string>& names)
{
    std::unordered_map<std::string, int> idx;
    for (int i = 0; i < (int)names.size(); ++i)
        idx[names[i]] = i;
    return idx;
}

// CS2 bone name table (indices 0-27, padded with placeholders)
inline const std::vector<std::string> CS2_BONE_NAMES = {
    "pelvis", "spine_0", "spine_1", "spine_2", "neck_0", "neck",
    "head", "b0", "b1", "right_shoulder", "right_elbow", "right_wrist",
    "b2", "left_shoulder", "left_elbow", "left_wrist", "b3", "b4",
    "b5", "b6", "b7", "b8", "left_hip", "left_knee", "left_ankle",
    "right_hip", "right_knee", "right_ankle"
};

inline const std::vector<std::pair<std::string, std::string>> SKELETON_EDGES = {
    { "head",           "neck"           },
    { "neck",           "spine_2"        },
    { "spine_2",        "spine_1"        },
    { "spine_1",        "spine_0"        },
    { "spine_0",        "pelvis"         },
    { "neck",           "left_shoulder"  },
    { "left_shoulder",  "left_elbow"     },
    { "left_elbow",     "left_wrist"     },
    { "neck",           "right_shoulder" },
    { "right_shoulder", "right_elbow"    },
    { "right_elbow",    "right_wrist"    },
    { "pelvis",         "left_hip"       },
    { "left_hip",       "left_knee"      },
    { "left_knee",      "left_ankle"     },
    { "pelvis",         "right_hip"      },
    { "right_hip",      "right_knee"     },
    { "right_knee",     "right_ankle"    },
};
