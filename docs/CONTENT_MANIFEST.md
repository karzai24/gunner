# Content manifest

| ID | Intended content | Unreal status/location |
|---|---|---|
| `character_vera` | Micah Vale / Field Warden; retain stable legacy ID | M0 placeholder `Content/Gunner/Characters/BP_Warden`, Epic Manny; no custom Micah art imported |
| `character_rook` | Rook Nadir / Ranger | Deferred; no second selectable pawn |
| `weapon_arc_carbine` | Arc Carbine hitscan rifle | M2, not implemented |
| `enemy_veyra_skitter` | Veyra Skitter melee pack hunter | M2, not implemented |
| `mode_rift_horde` | Three-wave co-op horde | M2/M4, not implemented |
| `level_rift_bastion` | Rift Bastion arena | M2; M0 `L_Foundation` is only a 44 x 32 m metric test room |
| `defense_pulse_fence` | Eight-second pulse slow/damage defense | M4, not implemented |
| `level_foundation` | Locomotion and camera fixture map | `Content/Gunner/Maps/L_Foundation` |

No old `.tscn` or `.gd` path is a runtime dependency. Stable IDs above are migration contracts, not an implemented selection/save system.
