#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <d3d11.h>
#include <memory>
#include <string>
#include <vector>
#include <cstdarg>
#include <cstdio>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "CS2Offsets.hpp"
#include "Overlay.hpp"
#include "ProcessMemoryReader.hpp"
#include "Skeleton.hpp"
#include "Drawing.hpp"

struct PlayerData {
    bool alive = false; int team = 0; int health = 0;
    std::vector<BoneVertex> bones;
    std::unordered_map<std::string, int> boneIndex;
};

static uintptr_t GetEntityPtr(const ProcessMemoryReader& r, uintptr_t gs, int index) {
    uintptr_t chunkPtr = r.readAbsolute<uintptr_t>(gs + 0x10 + 0x8 * (index >> 9));
    if (!chunkPtr) return 0;
    return r.readAbsolute<uintptr_t>(chunkPtr + 0x78 * (index & 0x1FF) + 0x70);
}

static float       g_dbgY = 60.f;
static ImDrawList* g_dl   = nullptr;
static void DBG(ImU32 col, const char* fmt, ...) {
    char buf[160];
    va_list a; va_start(a, fmt); vsnprintf(buf, sizeof(buf), fmt, a); va_end(a);
    g_dl->AddText({ 10.f, g_dbgY }, col, buf);
    g_dbgY += 14.f;
}

static bool s_boneLogged = false;

static std::vector<PlayerData> ReadPlayers(
    const ProcessMemoryReader& reader,
    uintptr_t localPawn,
    const glm::mat4& viewProj, float screenW, float screenH)
{
    std::vector<PlayerData> players;
    g_dbgY = 60.f;

    uintptr_t gs = reader.readClient<uintptr_t>(CS2::dwEntityList);
    if (!gs) { DBG(IM_COL32(255,80,80,255), "gs null!"); return players; }

    int checked = 0, dead = 0, boneFail = 0;
    int shownAlive = 0;

    for (int i = 1; i <= 1024; ++i) {
        uintptr_t ptr = GetEntityPtr(reader, gs, i);
        if (!ptr || ptr == localPawn) continue;
        checked++;

        int health = reader.readAbsolute<int32_t>(ptr + CS2::OFFSET_HEALTH);
        int team   = reader.readAbsolute<int32_t>(ptr + CS2::OFFSET_TEAM);

        if (health < 1 || health > 100) { dead++; continue; }

        // Show first 5 alive entities for debug
        if (shownAlive < 5) {
            DBG(IM_COL32(200,200,255,255), "idx:%d hp:%d t:%d", i, health, team);
            shownAlive++;
        }

        // NOTE: Team filtering temporarily disabled - OFFSET_TEAM needs re-verification
        // All alive entities (hp 1-100) that are not localPawn are rendered

        uintptr_t sn = reader.readAbsolute<uintptr_t>(ptr + CS2::OFFSET_GAME_SCENE_NODE);
        if (!sn) { boneFail++; continue; }

        uintptr_t bonePtr = reader.readAbsolute<uintptr_t>(
            sn + CS2::OFFSET_MODEL_STATE + CS2::OFFSET_BONE_MATRIX_PTR);
        if (!bonePtr) { boneFail++; continue; }

        // Dump raw bone float data once
        if (!s_boneLogged) {
            s_boneLogged = true;
            FILE* f = fopen("cs2_bonedata.txt", "w");
            if (f) {
                fprintf(f, "bonePtr: 0x%llX\n\n", bonePtr);
                uint8_t raw[480] = {};
                reader.readRaw(bonePtr, raw, sizeof(raw));
                fprintf(f, "=== 3x4 stride (48 bytes per bone) ===\n");
                for (int b = 0; b < 10; ++b) {
                    float* fl = reinterpret_cast<float*>(raw + b * 48);
                    fprintf(f, "Bone[%d] (+0x%X):\n", b, b * 48);
                    fprintf(f, "  floats: ");
                    for (int k = 0; k < 12; ++k)
                        fprintf(f, "%.3f ", fl[k]);
                    fprintf(f, "\n");
                    fprintf(f, "  tx=fl[3]:%.3f  ty=fl[7]:%.3f  tz=fl[11]:%.3f\n\n",
                        fl[3], fl[7], fl[11]);
                }
                fprintf(f, "\n=== 4x4 stride (64 bytes per bone) ===\n");
                uint8_t raw2[640] = {};
                reader.readRaw(bonePtr, raw2, sizeof(raw2));
                for (int b = 0; b < 10; ++b) {
                    float* fl = reinterpret_cast<float*>(raw2 + b * 64);
                    fprintf(f, "Bone[%d] (+0x%X):\n", b, b * 64);
                    fprintf(f, "  floats: ");
                    for (int k = 0; k < 16; ++k)
                        fprintf(f, "%.3f ", fl[k]);
                    fprintf(f, "\n");
                    fprintf(f, "  fl[12]:%.3f fl[13]:%.3f fl[14]:%.3f\n\n",
                        fl[12], fl[13], fl[14]);
                }
                fclose(f);
            }
        }

        auto matrices = reader.readBoneMatrices(bonePtr, CS2::BONE_COUNT);
        if (matrices.empty()) { boneFail++; continue; }

        PlayerData pd;
        pd.health    = health;
        pd.team      = team;
        pd.alive     = true;
        pd.bones     = ExtractAllBonePositions(matrices);
        pd.boneIndex = BuildBoneIndex(CS2_BONE_NAMES);
        ProjectBones(pd.bones, viewProj, screenW, screenH);
        players.push_back(std::move(pd));
    }

    DBG(IM_COL32(255,165,0,255), "chk:%d dead:%d bone:%d found:%d",
        checked, dead, boneFail, (int)players.size());
    DBG(IM_COL32(100,255,100,255), s_boneLogged ? "BONE LOGGED" : "waiting...");

    return players;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) return true;
    switch (msg) {
        case WM_NCHITTEST: return HTTRANSPARENT;
        case WM_DESTROY:   PostQuitMessage(0); return 0;
        case WM_SIZE:
            if (g_pd3dDevice && wParam != SIZE_MINIMIZED) {
                CleanupRenderTarget();
                g_pSwapChain->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam),
                                            DXGI_FORMAT_UNKNOWN, 0);
                CreateRenderTarget();
            }
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void RenderFrame(ProcessMemoryReader& reader, float screenW, float screenH) {
    const float clear[4] = { 0.f, 0.f, 0.f, 0.f };
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear);
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    g_dl = ImGui::GetBackgroundDrawList();

    try {
        // CS2 view matrix is row-major in memory, GLM is column-major - must transpose
        glm::mat4 viewProj = glm::transpose(reader.readClient<glm::mat4>(CS2::dwViewMatrix));

        uintptr_t localPawn = reader.readClient<uintptr_t>(CS2::dwLocalPlayerPawn);

        g_dl->AddText({ 10.f, 10.f }, IM_COL32(0,255,0,255), "CS2 Overlay Active");

        auto players = ReadPlayers(reader, localPawn, viewProj, screenW, screenH);

        g_dl->AddText({ 10.f, 44.f }, IM_COL32(200,200,200,255),
            ("Enemies: " + std::to_string(players.size())).c_str());

        for (const auto& player : players) {
            int onScreen = 0;

            // Draw all bone dots
            for (const auto& bone : player.bones) {
                if (!bone.screenPos.has_value()) continue;
                float bx = bone.screenPos->x;
                float by = bone.screenPos->y;
                if (bx < 0 || bx > screenW || by < 0 || by > screenH) continue;
                onScreen++;
                // Larger dots (6px) so they are clearly visible
                g_dl->AddCircleFilled({ bx, by }, 6.f, IM_COL32(255, 255, 0, 255));
            }

            // Draw skeleton lines
            DrawStickFigureOverlay(player.bones, player.boneIndex);

            // Draw bounding box around all on-screen bones
            DrawSkeletonBoundingBox(player.bones,
                IM_COL32(255, 50, 50, 220), 2.0f);

            // Draw hp label above bounding box using head bone (index 6)
            // Fall back to first on-screen bone if head not available
            std::optional<glm::vec2> labelPos;
            if (player.boneIndex.count("head")) {
                int hi = player.boneIndex.at("head");
                if (hi < (int)player.bones.size() && player.bones[hi].screenPos.has_value())
                    labelPos = player.bones[hi].screenPos;
            }
            if (!labelPos) {
                for (const auto& bone : player.bones) {
                    if (bone.screenPos.has_value()) { labelPos = bone.screenPos; break; }
                }
            }
            if (labelPos) {
                g_dl->AddText(
                    { labelPos->x - 10.f, labelPos->y - 20.f },
                    IM_COL32(255, 100, 100, 255),
                    (std::to_string(player.health) + "hp").c_str());
            }

            DBG(IM_COL32(180,255,180,255), "hp:%d bones_on:%d", player.health, onScreen);
        }

    } catch (const std::exception& e) {
        g_dl->AddText({ 10.f, 10.f }, IM_COL32(255,50,50,255),
            (std::string("[ERR] ") + e.what()).c_str());
    }

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    g_pSwapChain->Present(1, 0);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    HWND hwnd = CreateOverlayWindow(hInstance);
    if (!hwnd) return 1;
    if (!InitD3D(hwnd)) { CleanupD3D(); return 1; }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    ImGui::StyleColorsDark();
    ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = ImVec4(0, 0, 0, 0);
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    std::unique_ptr<ProcessMemoryReader> reader;
    try {
        reader = std::make_unique<ProcessMemoryReader>(L"cs2.exe");
    } catch (const std::exception& e) {
        MessageBoxA(nullptr, e.what(), "Attach Failed", MB_ICONERROR);
        return 1;
    }

    float screenW = (float)GetSystemMetrics(SM_CXSCREEN);
    float screenH = (float)GetSystemMetrics(SM_CYSCREEN);

    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg); DispatchMessage(&msg); continue;
        }
        RenderFrame(*reader, screenW, screenH);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupD3D();
    DestroyWindow(hwnd);
    UnregisterClassW(L"CS2OverlayClass", hInstance);
    return 0;
}
