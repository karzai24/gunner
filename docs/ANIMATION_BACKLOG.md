# Available animation backlog

Inventory updated 2026-09-20 after the user's question about unused animations. This records available material and remaining work; it does not authorize new gameplay scope or claim these actions are implemented. The current low-cover blind-fire work uses procedural arm composition over the acquired crouch pose and has passed its focused live checks; it is procedural composition, not an acquired blind-fire clip.

The motion-polish pass integrates rifle/pistol takeoff and additive landing recovery, both dry-fire clips, crouched equip composition, and the retargeted cross punch. Additional phases and actions below remain candidates; asset presence alone does not justify enabling them.

## Installed Epic clips

Paths below are relative to `Content/Characters/Mannequins/Anims/`; each listed clip is a `.uasset`. These installed Examples already target the canonical skeleton. Their presence does not establish action-state, collision or visual acceptance.

| Installed set | Exact paths / filenames | Remaining work |
|---|---|---|
| Rifle jump transitions — 5 | `Rifle/Jump/MM_Rifle_Jump_Start`, `MM_Rifle_Jump_Start_Loop`, `MM_Rifle_Jump_Apex`, `MM_Rifle_Jump_Fall_Land`, `MM_Rifle_Jump_RecoveryAdditive` | Start and RecoveryAdditive are integrated through project-owned root-locked copies. Start_Loop, Apex and Fall_Land remain unused; CharacterMovement selects the existing fall loop between takeoff and actual landing. |
| Pistol jump transitions — 2 | `Pistol/Jump/MM_Pistol_Jump_Start`, `MM_Pistol_Jump_RecoveryAdditive` | Start and RecoveryAdditive integrated through root-locked copies; existing fall loop retained. |
| Dry fire — 2 | `Rifle/MM_Rifle_DryFire`, `Pistol/MM_Pistol_DryFire` | Integrated: rate-limited empty trigger, protective crouch arm layer, immediate reload interruption. |
| Melee alternatives — 4 | `Unarmed/Attack/MM_Attack_01`, `MM_Attack_02`, `MM_Attack_03`, `MM_ChargedAttack` | Choose useful variations; validate contact, timing, interruption and sequencing. Current melee alternates the Quaternius jab and cross. |
| Hit reactions — 8 | Under `Rifle/HitReact/`: `MM_HitReact_Back_Med_01`, `MM_HitReact_Front_Hvy_01`, `MM_HitReact_Front_Lgt_01`, `MM_HitReact_Front_Lgt_02`, `MM_HitReact_Front_Lgt_03`, `MM_HitReact_Front_Lgt_04`, `MM_HitReact_Front_Med_01`, `MM_HitReact_Front_Med_02` | No player receiving-damage system currently invokes these. |
| Deaths — 6 | Under `Death/`: `MM_Death_Back_01`, `MM_Death_Front_01`, `MM_Death_Front_02`, `MM_Death_Front_03`, `MM_Death_Left_01`, `MM_Death_Right_01` | No player death/life-state system currently invokes these. |
| Other movement candidates — 2 | `Unarmed/Jump/MM_Dash`, `MM_WallJump` | Separate actions requiring movement/collision rules; wall jumping is not part of the current cover-shooter sandbox. |

The table originally identified 29 unused Epic clips; six are now selected (two starts, two recoveries, two dry-fire clips), leaving 23 of these candidates unused. The installed unarmed idle, eight-direction walk/jog and `MM_Jump`/`MM_Fall_Loop`/`MM_Land` are additional alternatives; the preserved foundation already uses the unarmed graph. Replacing the armed set with every unarmed alternative would not add a new mechanic. Epic's supplied `MM_Pistol_Fire_Montage` is also an unused wrapper around firing coverage already composed into the project's own montage.

## Quaternius UAL1: imported and partly retargeted

All **43 source animations** are imported under `Content/Gunner/Animation/Source/Quaternius/UAL1_Standard/SkeletalMeshes/`, named `UAL1_Standard<clip>.uasset`. The retained CC0 source and complete names are in [animation_inventory.json](../ArtSource/Quaternius/UAL_Standard_v3/animation_inventory.json).

**Seven** Manny outputs exist under `Content/Gunner/Animation/Retargeted/Manny/`. Six are selected: crouch idle, crouch forward, sprint, jab, cross and roll. One remains dormant:

- `A_UAL_Sword_Attack.uasset`: a sword attack, not verified knife handling. No sword item/action is composed.

The other **36 source clips have no Manny retarget output**. Relevant unused source candidates include:

| Purpose | Exact source clip names | Why not currently selected |
|---|---|---|
| Jump | `Jump_Start`, `Jump_Loop`, `Jump_Land` | Alternative to installed Epic jump coverage; retarget and transition work required. |
| Reactions/death | `Hit_Chest`, `Hit_Head`, `Death01` | Retargeting and a receiving-damage/life-state system required. |
| Interactions | `Interact`, `PickUp_Table`, `Push_Loop`, `Fixing_Kneeling` | Possible interaction, pickup, pushing or repair motions; corresponding gameplay is absent. |
| Pistol alternatives | `Pistol_Aim_Down`, `Pistol_Aim_Neutral`, `Pistol_Aim_Up`, `Pistol_Idle_Loop`, `Pistol_Reload`, `Pistol_Shoot` | Epic already supplies the selected pistol set. These are not proof of blind-fire or crouched handling. |
| Other alternatives | `Sword_Idle`, `Idle_Loop`, `Walk_Loop`, `Jog_Fwd_Loop` | Sword action is absent; generic locomotion would duplicate existing armed coverage. |

## Quaternius UAL2: downloaded research only

All **43 clips** remain in the local source `Saved/AssetResearch/Quaternius/UAL2_Standard_v2_1/source/Universal Animation Library 2[Standard]/Unreal-Godot/UAL2_Standard.glb`. None is imported or retargeted into gameplay. The source is excluded from Git; its complete [research inventory](evidence/motion/ual2_research_inventory.json) is committed. The inventory's original `source_file` field predates the move into `Saved`; the path above is current.

Relevant candidates are `Slide_Start`, `Slide_Loop`, `Slide_Exit`, `ClimbUp_1m`, `Melee_Hook`, `Melee_Hook_Rec`, `Hit_Knockback`, `OverhandThrow`, `LayToIdle`, `Chest_Open`, `Consume`, and `Walk_Carry_Loop`. They need import, retarget, pose inspection and the corresponding action rules. A 1 m climb is not a through-vault; a get-up clip is not a complete incapacitation/revival set; a throw needs an actual throwable mechanic. Sword, shield and ninja-jump sets also exist in the inventory but are not composed into this rifle/pistol sandbox.

## Local installed/Mixamo expansion

The local profile selects seven compatible installed Epic crouch clips (idle, four directions, entry and exit), and four Mixamo clips for hunched rifle sprint and standing rifle wall idle/left/right. Stationary free/high-cover crouch entry/exit composition is now implemented behind an optional graph capability at 2.5x playback, with movement/aim/action interruption and immediate protected-low-cover bypass; arm-only weapon carry also preserves the authored torso/legs through free/high-cover crouch and transitions. The 30-scenario rendered traversal suite passed both sessions, including stance/carry and protected-low-cover bypass. VaultOver remains dormant. Eleven Mixamo animations have been retargeted in total; seven remain dormant: Taking Cover, Standing Cover Turn, Crouch Cover To Cover, Vault Over Box, Stand Cover To Look, Emerge From Cover and Emerging. Their presence does not enable corners, transfers, vaults or entry/exit actions. The [transition fit report](research/TRANSITION_CLIP_FIT.md) records the remaining geometry, pose and weapon work.

The moving installed crouch poses are too tall for the 115 cm low-cover fixture, so the protected low-cover graph keeps Quaternius motion. The unarmed transfer also rises above protected cover height. The installed vault's full launch does not fit the 52 cm attached anchor. These are concrete integration gaps, not arbitrary omissions. See [MOVEMENT_ASSET_COVERAGE](research/MOVEMENT_ASSET_COVERAGE.md) for the source audit and [LOCAL_MOVEMENT_SETUP](LOCAL_MOVEMENT_SETUP.md) for reproduction.

## Genuine gaps and selection boundaries

No integrated source establishes a braced knee slide into cover, complete inner/outer corner and cover-transfer set, attached low-cover vault, authored directional roll variants, pistol wall poses, full sprint start/stop/turn set or knife-specific handling. Some entry/exit/corner/vault candidates have now been acquired but still need route/contact/weapon acceptance. Paid packs remain research only. Blind fire, crouched reload and crouched equip/dry fire use project-authored composition over existing clips; they do not supply dedicated source clips.

The 43-clip packs also include reference poses and unrelated dancing, driving, farming, swimming, conversation and fantasy actions. Importing a whole source library preserves options; enabling every clip would add unrelated mechanics and duplicate existing coverage. Keep useful omissions distinct from genuinely missing sources. See [ASSET_PROVENANCE](ASSET_PROVENANCE.md) for Epic Examples and Quaternius CC0 rights, and [TEST_MATRIX](TEST_MATRIX.md) for actual integration acceptance.

The earlier local traversal pass predates a restart-triggered auto-reimport that corrupted Mixamo target poses. Source storage/import metadata are now repaired, and the fresh recovery gate passed 2,013 poses across eleven clips and fourteen package hashes. The recovered 30-scenario traversal, movement, blind-fire, crouched-reload and polish suites now pass. Portable fallback also passed 184 checks; post-normal-editor persistence revalidated all 2,013 poses and fourteen package hashes with zero errors/warnings. Final acceptance and limitations are recorded in TEST_MATRIX.
