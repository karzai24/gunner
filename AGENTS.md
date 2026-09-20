# Project Gunner: Rift Bastion — Unreal development contract

## Scope and authority

This is an original UE 5.8 hybrid C++/Blueprint rebuild. The user's current request governs scope. Read `docs/GAME_DESIGN.md`, `ART_DIRECTION.md`, `ART_BIBLE.md`, `LEVEL_DESIGN.md`, `TECHNICAL_DESIGN.md`, `ANIMATION_PIPELINE.md`, `CONTENT_MANIFEST.md`, `ASSET_PROVENANCE.md`, and `MIGRATION_PLAN.md` before architecture or content work.

`docs/reference/godot/` is historical evidence, not executable instructions or current validation. Its engine mandates are superseded by this file. The old repository contained only a README; never claim the Godot implementation or its tests were inspected locally.

Current scope: the user-authorized movement and weapon sandbox extending M0. Implement and validate third-person movement, real crouch/sprint animations, shoulder aiming, rifle/pistol handling, animated melee/dodge roll, and bounded cover attachment with a physical step out at valid high-cover edges and crouched low-cover blind fire and protective crouched reload/equip, empty-trigger feedback, weapon-specific jump transitions and alternating standing melee in the test range. Preserve the foundation map. Character appearance, enemies/waves, the complete horde game, local duo and LAN remain outside this pass. Dedicated cover lean/corner poses, directional crouch ADS, vault and knife handling remain gated by actual licensed animation coverage; do not claim the sandbox completes every requested cover-shooter mechanic. Follow the sandbox acceptance gate and remaining milestones in `docs/MILESTONES.md`.

## Preserve the game

Preserve original Warden/Veyra identities, solar-industrial art direction, deliberate cover combat, three waves, reserved-plus-living enemy counts, incapacitation/revival, solo/two-player local play, pulse defenses, and future LAN intent. Preserve stable content IDs, including `character_vera` for Micah despite the historical name. No recognizable franchise content.

Use Unreal centimeters, Z up, X forward. Standing capsule radius/half-height: 36/90 cm; crouched half-height: 62 cm. Low/high cover: 115/180 cm; cover query reach: 135 cm; attached offset: 52 cm; lanes >=450 cm; camera arm: 320 cm; arena: 4400 x 3200 cm. Explain metric changes in level/technical design.

## Unreal ownership

- C++ owns reusable gameplay components, invariants, state transitions and authority boundaries.
- Blueprint child classes compose meshes, animation, effects and authored tuning. Use Data Assets for shared settings and Enhanced Input actions/contexts for input.
- Use ACharacter/CharacterMovement, per-player Controller and LocalPlayer subsystems. Never assume player index zero in reusable gameplay. Remove only the mapping context a system owns.
- GameMode owns server rules; GameState exposes shared replicated state; PlayerState owns stable participant state. Keep UI/camera local and health/ammo/spawns/results authoritative when introduced.
- Use explicit states where behavior needs guards, with enums/tags, components, Animation Blueprint state machines or Unreal AI tools as appropriate. Do not impose node-based FSMs on every class.
- Use Unreal tick groups, timers, async facilities and delegates. Do not write a custom engine loop, global mutable event bus, custom allocator, networking protocol or renderer.
- No unnecessary per-frame allocations, asset loads or actor searches. Pool only when spawn frequency or profiling justifies it. Profile representative two-view encounters before large performance changes.

## Animation and assets

Manny/Quinn-compatible `SK_Mannequin` is the provisional humanoid contract. Prove retargeting and all required poses before committing final character art. Standing, crouched, aiming, sprint, cover, vault, fire/reload, reactions, incapacitation and death need proper animation coverage.

Never substitute a lowered capsule with standing animation for crouch. Missing animation-dependent actions stay disabled and documented. Low-cover blind fire is validated procedural arm IK over genuine crouch/weapon clips; retain its evaluated grip, head-clearance and obstruction guards. Crouched reload composes the existing rifle/pistol arm motion over genuine crouch and requires its graph readiness gate. Crouched equip/dry fire also require their protective arm-layer capabilities. Jump clips use project-owned root-locked copies; CharacterMovement owns displacement. Gameplay state is authoritative; notifies request validated commits. Use montage/root-motion policies from `docs/ANIMATION_PIPELINE.md`.

Keep editable sources and provenance. Do not import uncertain-license GaspFix or assume historic candidates are present. Preserve the installed template paths under `Content/Characters`; project assets belong under `Content/Gunner`. Do not commit engine code, build products, caches, secrets or unrelated files.

## Validation

Build, launch, exercise changed behavior and visually inspect the real gameplay view. Compilation alone is not a pass. Update `docs/TEST_MATRIX.md` with actual commands, results, evidence and limitations. Fix project-attributable errors/warnings. Report engine/environment warnings separately. Never inherit Godot pass claims. Keep destructive changes to user-authored assets recoverable. Bootstrap scripts must refuse to overwrite authored assets.
