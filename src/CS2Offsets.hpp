#pragma once
#include <cstdint>

namespace CS2 {
    // From offsets.hpp (2026-05-07 dump - https://github.com/a2x/cs2-dumper)
    constexpr uintptr_t dwViewMatrix          = 0x2330AE0;
    constexpr uintptr_t dwEntityList          = 0x24D0DC0;
    constexpr uintptr_t dwLocalPlayerPawn     = 0x2056700;
    constexpr uintptr_t dwLocalPlayerCtrl     = 0x230A4F0;
    constexpr uintptr_t dwViewAngles          = 0x2340288;
    constexpr uintptr_t dwGlowManager         = 0x2327D40;

    // C_BaseEntity (client_dll.hpp 2026-05-07 dump)
    constexpr uintptr_t OFFSET_HEALTH          = 0x34C;  // m_iHealth        int32
    constexpr uintptr_t OFFSET_TEAM            = 0x3EB;  // m_iTeamNum       uint8
    constexpr uintptr_t OFFSET_GAME_SCENE_NODE = 0x330;  // m_pGameSceneNode CGameSceneNode*

    // CSkeletonInstance (parent: CGameSceneNode)
    constexpr uintptr_t OFFSET_MODEL_STATE     = 0x150;  // m_modelState     CModelState

    // CModelState - bone matrix pointer
    constexpr uintptr_t OFFSET_BONE_MATRIX_PTR = 0x80;

    constexpr int    MAX_PLAYERS = 64;
    constexpr size_t BONE_COUNT  = 256; // Read all bones
}
