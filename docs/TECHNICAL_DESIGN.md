# Unreal technical design

## Scope and inspected baseline

Engine: Epic Launcher UE 5.8.2 (CL 56702186), Apple Silicon macOS 26.6.2, Xcode 26.6, Mac SDK 26.5 and installed Metal toolchain. `Gunner.uproject` has the runtime `Gunner` module, editor-only `GunnerEditor` authoring module, and Game/Editor targets. AndroidFileServer remains disabled. The former remote contained only a README; historical Godot documents do not certify any current implementation.

The user authorized a **movement and weapon sandbox** extending M0. The default map is `/Game/Gunner/Motion/Maps/L_MotionRange`, derived from the preserved 44 x 32 m `/Game/Gunner/Maps/L_Foundation`. It adds three resetting targets and composes movement, dodge roll, aiming, weapons, melee and bounded static-wall attachment with physical edge exposure. No enemy AI, waves, player health/incapacitation, match loop, menu, local duo or LAN is implemented in this pass. Character appearance stays provisional.

Implementation and asset composition are ready for live verification; actual build/play/visual acceptance belongs in `TEST_MATRIX.md`. A generated asset or a passing compiler does not complete a milestone.

## Runtime and authoring ownership

| Owner / location | Responsibility |
|---|---|
| `AGunnerCharacter` | ACharacter/CharacterMovement, capsule, shoulder camera, movement/stance guards, sprint, contextual traversal and input handlers |
| `AGunnerPlayerController` | Adds/removes only its own Enhanced Input context in the owning LocalPlayer subsystem across possession, acknowledgment, setup and teardown |
| `UGunnerCombatComponent` | Explicit Idle/Firing/Reloading/Equipping/Melee states; per-weapon ammo; rate limits; montage lifecycle; obstruction traces; guarded target damage |
| `UGunnerCoverComponent` | Bounded static-wall query, attachment normal, low/high classification, movement constraint, edge checks, guarded step-out/return states and detach cleanup |
| `UGunnerDodgeComponent` | Validates grounded roll route/standing clearance, plays the acquired montage, applies a native CharacterMovement root-motion source and cleans it up on completion/interruption |
| `UGunnerAnimInstance` | Game-thread snapshot of velocity, stance, air/aim/sprint/cover and weapon selection for graph evaluation; no independent gameplay decisions |
| `UGunnerInputConfig` | Authored actions/context, isolated per player; no runtime key construction |
| `UGunnerMotionSettings` | Movement speeds, aim camera tuning and authoring gates for actual animation coverage |
| `UGunnerWeaponData` | Identity, static visual, grip/muzzle offsets, montages and weapon tuning; mutable ammunition belongs to combat component |
| `AGunnerTarget` | Authority-owned range durability and timed reset; fixture only, not an enemy or health framework |
| `AGunnerHUD` | Owning-controller canvas presentation of reticle, hit/obstruction feedback, ammunition and action state |
| `AGunnerGameMode` / `BP_MotionGameMode` | Pawn/controller/HUD composition; opt-in development probes |
| `Content/Gunner/Motion` | Range map, BP_WardenMotion, weapon/motion/input Data Assets, graph, blend spaces and montages |
| `Content/Gunner/Animation` | Separate imported Quaternius source, project IK rigs/retargeter and Manny retarget outputs |
| `Content/Characters` / `Content/Weapons` | Installed Epic template package paths; provenance retained |
| `Source/GunnerEditor` | Editor-only native helpers to author/compile Blend Spaces and AnimGraph; no UnrealEd dependency in runtime module |
| `Tools` / `ArtSource` | Guarded reproducible authoring scripts, local launch/build tools, retained editable animation source and licenses |

Keyboard movement axes accumulate so opposing keys cancel; CharacterMovement limits diagonal speed. Mouse input is a delta, stick input is a rate scaled by delta time with a dead zone. Legacy controller scales are disabled. Positive mouse/right-stick Y means look up; pitch limits are -60/+50 degrees. Controller mappings exist for one local player; physical device and duo routing acceptance is separate.

Mac input compatibility: `Config/Mac/MacEngine.ini` sets `Slate.MacUseNewMouseControllerMovement=0` in Engine `[ConsoleVariables]`. The installed 5.8.2 `MacApplication.cpp` implements this switch: false uses AppKit deltas in normal and high-precision capture, while true uses `FAppleMouseController` for captured movement. The setting is read-only and must load before `FMacApplication` is created, so it requires an editor/game restart. This is a targeted workaround for the reported physical mouse freeze after ADS; the precise hardware failure has not been reproduced by automation. ADS itself does not disable look or change input mode. The opt-in `-GunnerLookSmoke` probe verifies the Enhanced Input/controller/camera path across repeated aim/release cycles, but bypasses the native hardware backend and cannot certify this workaround on the user's mouse.

## Movement and camera

The capsule is radius 36/standing half-height 90 cm and crouched half-height 62 cm. Crouch/stand uses native CharacterMovement clearance handling. The animation graph contains a real crouch idle/forward cycle. Because the acquired set has no sideways/backward crouch clips, free crouched movement faces travel, while crouched ADS rejects movement input. The authoring flag `bDirectionalCrouchReady` remains false. Never enable fixed-facing crouch strafing by renaming the forward gait.

The armed upper-body layer fades to zero during non-ADS crouch so its upright torso cannot replace the imported protective crouch pose. Standing and crouched ADS restore that layer; sprint also fades it out. This is runtime blending of existing clips, with no manually keyed torso correction. Crouch input rejects an active reload or equip action.

Current Data Asset defaults are 380 cm/s travel, 190 aim, 650 sprint and 140 crouch. Cover travel uses 150 cm/s, with the native crouch speed applying when crouched. Acceleration is 1200 cm/s², braking 1600 cm/s², rotation 480 degrees/s, jump velocity 440 cm/s and air control 0.2. These are prototype tuning choices, not values recovered from Godot.

Normal camera: 320 cm arm, 50 cm shoulder offset and 75-degree FOV. Aim camera: 220 cm arm and 58-degree FOV. Sprint camera: 350 cm arm and 82-degree FOV. Vertical target offset blends from 65 cm standing to 45 cm crouched; shoulder swaps interpolate. Spring-arm sweeps use a 12 cm probe. These camera changes preserve the original 320 cm normal baseline.

Sprint requires forward input, grounded standing movement, no aim, no roll and no cover. Sprint, roll and falling block combat. Melee locks travel; low-cover and active-peek melee are rejected, and crouched/pending-crouch melee is disabled because its acquired jab is a standing full-body action. Space first detaches if attached, otherwise tries cover in camera-yaw direction, then falls back to grounded jump. J requests jump without a cover search. E/controller left shoulder requests the separate dodge roll. There is no vault branch.

The roll uses `AM_DodgeRoll`, the acquired in-place UAL Roll at a montage rate scale of 1.65. `UGunnerDodgeComponent` selects current travel direction above 30 cm/s, otherwise camera yaw, and validates up to 350 cm of travel. A standing-capsule sweep may shorten the route; less than 100 cm rejects. Floor samples along the route and full destination clearance reject gaps, non-static support and excessive height differences. A native `FRootMotionSource_MoveToForce` drives CharacterMovement over the montage duration; the animation track itself remains in-place. Crouch, cover, reload, equip and melee reject a roll. Completion, interruption, unpossession, teardown, timeout or leaving walking removes the motion source and releases state. The capsule remains standing-sized throughout. During the roll, the camera arm extends to 350 cm and its target height drops from 65 to 5 cm above capsule center, interpolating back afterward to keep the low pose in view.

## Cover geometry and current limits

`UGunnerCoverComponent` queries at most 135 cm, accepts a near-vertical static wall, rejects ankle-height objects using a body-height trace, and classifies low/high using a higher trace. It sweeps the capsule to a 52 cm offset, then enables a native CharacterMovement plane constraint. Failed snaps do not teleport through geometry. Attached wall validity is normally checked on a 0.04-second component interval; step-out/return transitions tick each frame. Detach, unpossession and teardown remove the constraint.

Movement projects onto the wall tangent and checks geometry ahead before allowing travel. Corners require explicit detach/re-attach. Low cover requests crouch while protected and standing for ADS; native standing clearance still applies. High cover has None/SteppingOut/Exposed/Returning states: ADS at the selected open edge requests a 70 cm lateral step, with capsule and supported-floor checks before moving. Existing armed directional locomotion animates the step. Releasing ADS or invalidating its selected side requests return to the stored cover anchor. Normal player movement is suppressed during this transition/exposure; a blocked return times out and detaches at the safe swept position. High-cover peeking is standing only while directional crouch coverage is missing. High-cover fire still requires ADS, an allowed edge and the final obstruction traces.

Low-cover blind fire is a separate LMB action without ADS. It preserves the actual crouched capsule and imported crouch torso/head. Native two-bone arm IK places both weapon grips above the measured static barricade top, and the existing additive fire clips supply recoil. The character faces the camera yaw and stops travel while raising/firing; free mouse look remains enabled. The local camera target rises to 90 cm above the crouched capsule center so the aiming ray can see over cover, then returns to its usual offset. RMB cancels blind fire and keeps the existing standing pop-up ADS.

The first blind shot queues until the evaluated pose clears the wall: arm blend >=0.95, firing hand/muzzle >top+4 cm, head below top-5 cm, support hand within 5 cm of its IK target, and view/body facing outward. A 0.9-second unsafe-pose timeout cancels without spending ammo. A brief LMB click queues one shot, the rifle repeats while held, and the pistol remains one shot per press. Releasing after a shot, detaching, unpossessing, losing cover or entering ADS cancels the action. Cancelled input hard-stops pending fire. The blind route checks torso-to-raised-hand and hand-to-muzzle geometry, then traces from the actual muzzle with a 4-degree spread cone; it never ignores the barricade or relocates the shot origin. Normal firing retains its direct torso-to-muzzle obstruction check. This is bounded procedural coverage for low static walls, not a new source animation pack or high-cover blind fire.

This provides a geometry/state prototype using the available armed/crouch movement and an actual step into exposure. It does not contain dedicated cover entry/exit, wall-lean, corner-turn or vault poses. Fixture heights remain 115/180 cm and the room remains 4400 x 3200 cm. Full M3 cover-animation acceptance is still outstanding.

## Weapon and melee actions

Rifle: 30-round magazine, 240 initial reserve, automatic fire every 0.12 seconds, 20 damage. Pistol: 12-round magazine, 120 reserve, one shot per press with a 0.27-second rate limit, 35 damage. Both default to 10000 cm range. Each weapon retains its own magazine/reserve through swaps. Weapon visuals are hand-attached static template meshes; separately moving magazines/bolts and production weapon effects/audio are not implemented.

A camera trace selects the aim point; the normal-fire body-to-muzzle check rejects a barrel protruding through geometry; a muzzle-to-target trace resolves the actual impact. Only `AGunnerTarget` fixtures receive sandbox damage. A blocked shot still consumes the fired round. Short-lived debug lines/points and a local HUD provide prototype feedback; no per-shot actor spawning is needed.

Fire/equip/reload require a valid animation and grounded authority. Reload commits a conserved magazine/reserve transfer only on uninterrupted montage completion. A serial token rejects stale callbacks, and a timeout clears locks without awarding ammo. Native montage delegates are the current completion contract; no unguarded notify changes ammunition. Equip starts the incoming montage and updates its selected visual after the montage starts successfully.

Reload supports actual/pending crouch and low-cover attachment when the assigned animation graph declares protective reload coverage. The graph layers only the existing reload arm branches onto the genuine crouch pose, keeping the pelvis, torso and head low. Starting reload stops fire/aim; cover peeking is suppressed during reload, and low-cover ADS returns to crouch until completion. Held ADS may resume afterward. The existing completion, cancellation, timeout and ammo-conservation rules apply unchanged. Weapon equip still rejects actual/pending crouch and all attached low-cover states because protective equip coverage is absent.

Melee uses the retargeted `Punch_Jab` full-body montage. Its single timer requests impact at 38% of duration; code validates authority, possession, current action and active montage before damage. A bounded 135 cm sweep with 35 cm radius, forward-angle gate and occlusion trace applies up to 40 damage to a range target. It is a standing jab, not knife combat. Timers/delegates are canceled during interruption, unpossession and teardown.

## Future authority boundaries

The sandbox uses native authority checks but does **not** implement replicated weapons, combat RPCs, predicted cover or network sessions. Local camera/HUD remain tied to their owner. Preserve these boundaries as later work expands:

| Future system | Planned owner |
|---|---|
| Waves, reservations and match rules | GameMode / wave director; GameState exposes shared state |
| Stable participant identity | PlayerState |
| Health, incapacitation and revive | Authority-owned replicated health/interaction components |
| Enemy perception/navigation | AIController and Unreal navigation/Behavior Tree |
| Pulse defenses | Authority-owned replicated defense actor/component |
| Local duo | LocalPlayers, device assignment, separate cameras/HUDs |
| LAN | Native listen-server replication and session facilities; extend predicted movement when required |

No custom allocator, renderer, global event bus, engine loop or networking protocol is introduced. Use ordinary engine timers, delegates and movement. Add pooling or heavier systems only after representative profiling.

Use the existing raster baseline without Lumen, virtual shadow maps or motion blur on the 16 GB development Mac. This is not a measured performance guarantee. Cooked, representative two-view profiling remains future acceptance work.

## References

- [Enhanced Input](https://dev.epicgames.com/documentation/unreal-engine/enhanced-input-in-unreal-engine)
- [Character movement](https://dev.epicgames.com/documentation/unreal-engine/coder-03-configure-character-movement-with-cplusplus-in-unreal-engine)
- [macOS requirements](https://dev.epicgames.com/documentation/en-us/unreal-engine/macos-development-requirements-for-unreal-engine)

Installed 5.8 headers/templates govern API details. See `ANIMATION_PIPELINE.md`, `ASSET_PROVENANCE.md` and `TEST_MATRIX.md` for coverage, source rights and observed verification.
