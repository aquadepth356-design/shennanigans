# CS2 Overlay

A DirectX 11 overlay for CS2 with ESP/skeleton rendering.

## Files

| File | Purpose |
|---|---|
| `src/main.cpp` | Entry point, render loop, entity scanning |
| `src/CS2Offsets.hpp` | All CS2 memory offsets (update when game patches) |
| `src/Skeleton.hpp` | Bone extraction, projection, bone name definitions |
| `src/Drawing.hpp` | ImGui drawing helpers (bounding box, skeleton lines) |
| `src/Overlay.hpp` | Win32 transparent window + D3D11 init |
| `src/ProcessMemoryReader.hpp` | ReadProcessMemory wrapper |
| `src/Math.hpp` | WorldToScreen, vector math |

## Offsets last updated
2026-05-07 dump from https://github.com/a2x/cs2-dumper

## Dependencies
- [ImGui](https://github.com/ocornut/imgui)
- [GLM](https://github.com/g-truc/glm)
- DirectX 11 SDK
