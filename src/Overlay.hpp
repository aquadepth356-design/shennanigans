#pragma once
#include <Windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <stdexcept>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

// ─── D3D11 globals ────────────────────────────────────────────────────────────

inline ID3D11Device*           g_pd3dDevice           = nullptr;
inline ID3D11DeviceContext*    g_pd3dDeviceContext    = nullptr;
inline IDXGISwapChain*         g_pSwapChain           = nullptr;
inline ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

inline void CreateRenderTarget() {
    ID3D11Texture2D* pBack = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBack));
    if (pBack) {
        g_pd3dDevice->CreateRenderTargetView(pBack, nullptr, &g_mainRenderTargetView);
        pBack->Release();
    }
}

inline void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

inline void CleanupD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain)        { g_pSwapChain->Release();        g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice)        { g_pd3dDevice->Release();        g_pd3dDevice = nullptr; }
}

// Forward declare WndProc (defined in main.cpp)
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// ─── Overlay window creation ──────────────────────────────────────────────────

inline HWND CreateOverlayWindow(HINSTANCE hInstance) {
    WNDCLASSEX wc    = { sizeof(WNDCLASSEX) };
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = L"CS2OverlayClass";
    RegisterClassEx(&wc);

    int W = GetSystemMetrics(SM_CXSCREEN);
    int H = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,
        L"CS2OverlayClass", L"CS2 Overlay",
        WS_POPUP,
        0, 0, W, H,
        nullptr, nullptr, hInstance, nullptr
    );

    // LWA_ALPHA = 255 -> fully opaque frame; per-pixel alpha handled by DX11 + DWM
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);

    // Extend DWM glass frame to entire client area -> enables true per-pixel alpha
    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    ShowWindow(hwnd, SW_SHOW);
    return hwnd;
}

// ─── D3D11 + alpha blend initialisation ──────────────────────────────────────

inline bool InitD3D(HWND hwnd) {
    int W = GetSystemMetrics(SM_CXSCREEN);
    int H = GetSystemMetrics(SM_CYSCREEN);

    DXGI_SWAP_CHAIN_DESC sd               = {};
    sd.BufferCount                        = 2;
    sd.BufferDesc.Width                   = (UINT)W;
    sd.BufferDesc.Height                  = (UINT)H;
    sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow                       = hwnd;
    sd.SampleDesc.Count                   = 1;
    sd.Windowed                           = TRUE;
    sd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL fl;
    const D3D_FEATURE_LEVEL flArr[] = { D3D_FEATURE_LEVEL_11_0 };

    if (FAILED(D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        flArr, 1, D3D11_SDK_VERSION,
        &sd, &g_pSwapChain, &g_pd3dDevice, &fl, &g_pd3dDeviceContext)))
        return false;

    // Alpha blending: SRC_ALPHA / INV_SRC_ALPHA - standard transparency
    D3D11_BLEND_DESC bd                              = {};
    bd.RenderTarget[0].BlendEnable                   = TRUE;
    bd.RenderTarget[0].SrcBlend                      = D3D11_BLEND_SRC_ALPHA;
    bd.RenderTarget[0].DestBlend                     = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOp                       = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].SrcBlendAlpha                 = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha                = D3D11_BLEND_ZERO;
    bd.RenderTarget[0].BlendOpAlpha                  = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].RenderTargetWriteMask         = D3D11_COLOR_WRITE_ENABLE_ALL;

    ID3D11BlendState* pBS = nullptr;
    g_pd3dDevice->CreateBlendState(&bd, &pBS);
    g_pd3dDeviceContext->OMSetBlendState(pBS, nullptr, 0xFFFFFFFF);
    pBS->Release();

    CreateRenderTarget();
    return true;
}
