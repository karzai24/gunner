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

Current melee alternates the retargeted standing jab and cross (`AM_MeleeJab`, `AM_MeleeCross`), shared by both weapon definitions. Dodge uses the retargeted `AM_DodgeRoll` full-body montage and a guarded native movement source. Both live under `Content/Gunner/Motion/Animation/Montages`. No knife item, knife-specific animation or vault action is claimed.

The crouch idle/forward clips retain their protective torso outside ADS by fading out the upright armed layer. Rifle and pistol reloads reuse the installed Epic clips through an arm-only layer over genuine crouch in `ABP_WardenMotionPolish`. Both work while crouched or attached to low cover. Equip and dry fire use an analogous protective arm layer during crouch/low cover; dedicated crouched handling source clips remain missing.

Animation assets are grouped under `Content/Gunner/Animation/Source`, `Animation/Rigs`, `Animation/Retargeted/Manny`, and the composed `Content/Gunner/Motion/Animation`. Original Epic character and weapon paths are retained under `Content/Characters` and `Content/Weapons`. See `ASSET_PROVENANCE.md` for creators, licenses, source hashes and modifications.

No old `.tscn` or `.gd` path is a runtime dependency. Stable IDs are content contracts, not an implemented character selection or save system.

Low-cover blind fire composes `ABP_WardenBlindFire`, native arm IK and sampled left-hand grips in the existing two weapon Data Assets. The prior animation graph is retained. Rifle/pistol use the actual raised muzzle while preserving protective crouch; no new character art or third-party blind-fire clip is imported.

`ABP_WardenMotionPolish` is the committed portable graph and includes prior blind-fire/reload composition. A compatible local profile may select `ABP_WardenCoverMovement` at spawn without changing that Blueprint package. All earlier graphs remain available. No new source animations are imported for crouched reload.

Motion polish adds four project-owned root-locked Epic copies under `Content/Gunner/Motion/Animation/InPlace`: `A_Rifle_JumpStart`, `A_Rifle_JumpLand`, `A_Pistol_JumpStart`, `A_Pistol_JumpLand`. Seven montages under `Animation/Montages` compose those copies, the two original Epic dry-fire clips, and `AM_MeleeCross`. Both weapon Data Assets reference their selected variants. `Tools/install_motion_polish.py` is guarded against overwriting any generated output and backs up the existing character/weapon packages.

## Optional local movement assets

These paths are ignored by Git and are not included with a fresh clone. Preparation scripts and metadata are committed; each developer must obtain the licensed sources locally. The foundation map and public character package retain no dependency on them.

| Local package family | Selected content | Dormant content |
|---|---|---|
| `Content/Gunner/LicensedLocal/MoverTraversal` | Seven selected canonical clips: idle/four directions in `BS_DirectionalCrouch`, plus stationary entry/exit evaluators; stance/carry rendered checks passed | `A_VaultOverCandidate`; intermediate traversal graph/settings retained for recovery |
| `Content/Gunner/LicensedLocal/Mixamo/Source` | Original X Bot import and eleven animation sources retained for editing/retargeting | Source mesh is not the game character |
| `Content/Gunner/LicensedLocal/Mixamo/Retargeted/Manny` | `A_Mixamo_CrouchedRun`, `CoverIdleRifle`, `CoverLeftRifle`, `CoverRightRifle` | `CoverEntryRifle`, `CoverTurn`, `CoverTransferCrouch`, `VaultBox`, `CoverLook`, `CoverExitLeft`, `CoverExitRightRifle` |
| `Content/Gunner/LicensedLocal/Mixamo/Runtime` | `ABP_WardenCoverMovement`, `BS_RifleWallMovement`, `DA_CoverMovement` | No corner, transfer, slide or vault action is activated |

[LOCAL_MOVEMENT_SETUP](LOCAL_MOVEMENT_SETUP.md) describes configuration, guards and reproduction. [ASSET_PROVENANCE](ASSET_PROVENANCE.md) records the separate source rights; imported candidates are not completed mechanics.

The first local FBX source layout triggered normal-editor auto-reimport and corrupted all eleven retargeted targets. The affected tree is preserved, and source FBX files now live outside Content with fresh generic empty import data on generated targets. A fresh recovery persistence gate passed 2,013 poses/eleven clips and fourteen package hashes. The local graph also adds arm-only carry through free/high-cover crouch and its stance transitions. The recovered traversal (612), movement (186), blind-fire (68), crouched-reload (224) and polish (242) checks passed with zero failures. Portable fallback also passed 184 checks; post-normal-editor persistence revalidated all 2,013 poses and fourteen package hashes with zero errors/warnings. Final acceptance and limitations are recorded in TEST_MATRIX.
