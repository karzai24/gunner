# Content manifest

The current playable composition is the movement/weapon range. Implemented content still requires the live acceptance recorded in `TEST_MATRIX.md`; asset presence alone is not a pass.

| ID | Intended content | Unreal status/location |
|---|---|---|
| `character_vera` | Micah Vale / Field Warden; retain stable legacy ID | Epic Manny placeholder; motion composition `Content/Gunner/Motion/Characters/BP_WardenMotion`; foundation `Content/Gunner/Characters/BP_Warden` preserved; no custom Micah art |
| `character_rook` | Rook Nadir / Ranger | Deferred; no second selectable pawn |
| `weapon_arc_carbine` | Arc Carbine hitscan rifle | Sandbox `Content/Gunner/Motion/Weapons/DA_Rifle`; Epic template rifle model; automatic fire, 30-round magazine and 240 reserve |
| `weapon_coil_pistol` | Coil Pistol hitscan sidearm | Sandbox `Content/Gunner/Motion/Weapons/DA_Pistol`; Epic template pistol model; semiautomatic fire, 12-round magazine and 120 reserve |
| `level_motion_range` | Movement, cover and weapon fixtures | Default `Content/Gunner/Motion/Maps/L_MotionRange`; 44 x 32 m room derived from foundation; three resetting range targets |
| `level_foundation` | Original locomotion and camera fixture map | Preserved `Content/Gunner/Maps/L_Foundation` |
| `enemy_veyra_skitter` | Veyra Skitter melee pack hunter | Deferred; range targets are not enemies |
| `mode_rift_horde` | Three-wave co-op horde | Deferred; no wave/match loop |
| `level_rift_bastion` | Rift Bastion arena | Deferred; test rooms are not the complete arena |
| `defense_pulse_fence` | Eight-second pulse slow/damage defense | Deferred |

The current melee action is a retargeted standing jab (`AM_MeleeJab`), shared by the two weapon definitions. Dodge uses the retargeted `AM_DodgeRoll` full-body montage and a guarded native movement source. Both live under `Content/Gunner/Motion/Animation/Montages`. No knife item, knife-specific animation or vault action is claimed.

The crouch idle/forward clips retain their protective torso outside ADS by fading out the upright armed layer. Rifle and pistol reload/equip clips are upright-only: those actions are blocked during actual/pending crouch and throughout low-cover attachment, including standing ADS. Dedicated crouched handling clips remain missing.

Animation assets are grouped under `Content/Gunner/Animation/Source`, `Animation/Rigs`, `Animation/Retargeted/Manny`, and the composed `Content/Gunner/Motion/Animation`. Original Epic character and weapon paths are retained under `Content/Characters` and `Content/Weapons`. See `ASSET_PROVENANCE.md` for creators, licenses, source hashes and modifications.

No old `.tscn` or `.gd` path is a runtime dependency. Stable IDs are content contracts, not an implemented character selection or save system.
