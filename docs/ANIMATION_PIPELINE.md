# Humanoid animation contract

## Decision

Use Epic's UE5 Manny/Quinn-compatible `SK_Mannequin` as the provisional gameplay skeleton. It is installed with UE 5.8.2, supports native template locomotion/Control Rig and gives incoming packs a known retarget target. Do not confuse it with UEFN or assume every asset named Manny is directly compatible. Custom Micah and Rook art waits until locomotion, crouch, weapon handling, cover and retargeting pass.

M0 composes `SKM_Manny_Simple` and the installed `ABP_Unarmed` on BP_Warden. The template Animation Blueprint contains a locomotion blend space and airborne states with foot IK; it casts to base Character rather than a template gameplay class. This is a temporary standing baseline, not a final armed Warden animation graph. Epic mannequin branding/materials are placeholder content.

## Contract to validate in M1

- Unreal centimeters, Z up; actor forward +X. Template skeletal component uses yaw -90° and feet at capsule bottom (mesh Z -90 cm). Inspect actual sole contact and proportions before altering scale.
- Keep root/pelvis hierarchy and template reference pose. Document any source T/A-pose correction in an IK Retargeter asset, not ad hoc component scale.
- Create project IK rigs and chain mappings for root/pelvis, spine, neck/head, arms/hands, legs/feet and IK goals. Keep source packs separate; retarget to the canonical skeleton and inspect clips in-engine.
- Retarget test set: idle, forward/back/lateral walk/run, crouch idle and moving, aim turns, sprint, vault and reload. Inspect feet, pelvis height, shoulder twist, hand contact, root drift and transition continuity.
- Create project Animation Blueprint after asset coverage is established. Base layer: standing/crouched locomotion, airborne, cover, incapacity/death. Upper-body slot and Layered Blend Per Bone: armed aim/fire/reload. Aim Offsets and hand IK preserve weapon alignment.
- Animation reads velocity, grounded state, stance, aim and gameplay action state. Avoid independently authoritative gameplay state in AnimBP. Animation notifies request code-validated reload/interaction commits, with interruption/time-out handling.
- Default locomotion is in-place driven by CharacterMovement. Use root motion only for deliberate montage actions (vault) with destination validation; future Motion Warping targets are server-approved. Test cancellation and replication before claiming network readiness.
- Planned project sockets: `weapon_r`, `weapon_back`; weapon asset sockets: `muzzle`, `grip_l`. These names are reserved, not currently authored or validated.

## Coverage ledger

| Requirement | Available evidence now | Acceptance status |
|---|---|---|
| Standing idle/walk/run, airborne | Installed unarmed AnimBP/blend space/clips, integrated M0 | Baseline; see TEST_MATRIX for live validation |
| Crouch idle and locomotion | No crouch clip in installed character set inspected | Missing; action disabled |
| Aim/weapon handling/fire/reload | Installed rifle/pistol candidates only | Not integrated or proven |
| Sprint | No distinct sprint integration | Pending |
| Cover entry/idle/move/exit/peek | No evaluated complete set | Missing |
| Vault | No validated vault montage/warping contract | Missing |
| Hit/incapacity/death | Some template death clips; no complete life-state set | Pending |
| Second-source retarget | Pipeline defined only | Not yet demonstrated |

Use licensed compatible temporary assets to fill gaps; no purchase/account action without authorization and no unclear-license imports. Do not use the historic GaspFix candidate. Record creator, source/version, license, date, paths, modifications and attribution before integration. M1 cannot be accepted by renaming clips, capsule-only crouch, import success or static screenshots alone.
