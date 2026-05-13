# Debug Notes

## Current Status
- Entity scanning working (enemies found)
- Bounding box visible on enemies
- Bone dots clustered — bone matrix layout needs confirming
- Team offset returns unexpected values in some modes

## Pending
- Paste `cs2_bonedata.txt` output to confirm bone stride (48 vs 64 byte)
- Confirm correct bone indices for head/neck/shoulders/hips/ankles
- Fix team filter after confirming team values from debug overlay

## Offset Source
https://github.com/a2x/cs2-dumper/tree/main/output  
Commit: `1919cb002b00d75bf6ade7fea33ed76f98d8397f`  
Dump date: 2026-05-07

## Key Offsets
| Field | Offset | Source |
|---|---|---|
| `m_iHealth` | `0x34C` | C_BaseEntity |
| `m_iTeamNum` | `0x3EB` | C_BaseEntity |
| `m_pGameSceneNode` | `0x330` | C_BaseEntity |
| `m_modelState` | `0x150` | CSkeletonInstance |
| bone matrix ptr | `+0x80` | CModelState |
| `dwEntityList` | `0x24D0DC0` | offsets.hpp |
| `dwLocalPlayerPawn` | `0x2056700` | offsets.hpp |
| `dwViewMatrix` | `0x2330AE0` | offsets.hpp |
