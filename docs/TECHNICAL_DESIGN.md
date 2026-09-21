# Unreal technical design

## Scope and inspected baseline

Engine: Epic Launcher UE 5.8.2 (CL 56702186), Apple Silicon macOS 26.6.2, Xcode 26.6, Mac SDK 26.5 and installed Metal toolchain. `Gunner.uproject` has the runtime `Gunner` module, editor-only `GunnerEditor` authoring module, and Game/Editor targets. AndroidFileServer remains disabled. The former remote contained only a README; historical Godot documents do not certify any current implementation.

The user authorized a **movement and weapon sandbox** extending M0. The default map is `/Game/Gunner/Motion/Maps/L_MotionRange`, derived from the preserved 44 x 32 m `/Game/Gunner/Maps/L_Foundation`. It adds three resetting targets and composes movement, dodge roll, aiming, weapons, melee and bounded static-wall attachment with physical edge exposure. No enemy AI, waves, player health/incapacitation, match loop, menu, local duo or LAN is implemented in this pass. Character appearance stays provisional.

An earlier local traversal composition completed 24 scenarios in each of two rendered PIE sessions with zero failures. A subsequent editor restart automatically reimported 23 Mixamo assets and corrupted the retargeted poses, invalidating that run as final acceptance. The source layout/import metadata have been repaired and the fresh recovery persistence gate passed 2,013 poses across eleven clips and fourteen package hashes. The recovered profile has passed 30 traversal scenarios in each of two rendered sessions (612 checks), plus movement (186), blind-fire (68), crouched-reload (224) and polish (242) regressions. Portable fallback also passed 184 checks; post-normal-editor persistence revalidated all 2,013 poses and fourteen package hashes with zero errors/warnings. Current build/play/visual evidence and limitations are recorded in `TEST_MATRIX.md`; a generated asset or passing compiler does not complete a milestone.

## Runtime and authoring ownership

| Owner / location | Responsibility |
|---|---|
| `AGunnerCharacter` | ACharacter/CharacterMovement, capsule, shoulder camera, movement/stance guards, sprint, contextual traversal and input handlers |
| `AGunnerPlayerController` | Adds/removes only its own Enhanced Input context in the owning LocalPlayer subsystem across possession, acknowledgment, setup and teardown |
| `UGunnerCombatComponent` | Explicit Idle/Firing/Reloading/Equipping/Melee states; per-weapon ammo; rate limits; montage lifecycle; obstruction traces; guarded target damage |
| `UGunnerCoverComponent` | Pure static-wall candidate query, swept supported approach, arrival/cancel ownership, low/high classification, bounded wall travel, guarded step-out/return and detach cleanup |
| `UGunnerDodgeComponent` | Validates grounded roll route/standing clearance, plays the acquired montage, applies a native CharacterMovement root-motion source and cleans it up on completion/interruption |
| `UGunnerAnimInstance` | Game-thread snapshot of velocity, stance, air/aim/sprint/cover and weapon selection for graph evaluation; no independent gameplay decisions |
| `UGunnerInputConfig` | Authored actions/context, isolated per player; no runtime key construction |
| `UGunnerMotionSettings` | Movement speeds, turn/hold tuning, animation capability gates and optional compatible local AnimationClass |
| `UGunnerWeaponData` | Identity, static visual, grip/muzzle offsets, montages and weapon tuning; mutable ammunition belongs to combat component |
| `AGunnerTarget` | Authority-owned range durability and timed reset; fixture only, not an enemy or health framework |
| `AGunnerHUD` | Owning-controller canvas presentation of reticle, hit/obstruction feedback, ammunition and action state |
| `AGunnerGameMode` / `BP_MotionGameMode` | Pawn/controller/HUD composition; opt-in development probes |
| `Content/Gunner/Motion` | Range map, BP_WardenMotion, weapon/motion/input Data Assets, graph, blend spaces and montages |
| `Content/Gunner/Animation` | Separate imported Quaternius source, project IK rigs/retargeter and Manny retarget outputs |
| `Content/Characters` / `Content/Weapons` | Installed Epic template package paths; provenance retained |
| `Source/GunnerEditor` | Editor-only native helpers to author/compile Blend Spaces and AnimGraph; no UnrealEd dependency in runtime module |
| `Tools` / `ArtSource` | Guarded reproducible authoring scripts, local launch/build tools, retained editable animation source and licenses |

Keyboard movement axes accumulate so opposing keys cancel; CharacterMovement limits diagonal speed. Mouse input is a delta, stick input is a rate scaled by delta time with a dead zone. Legacy controller scales are disabled. Positive mouse Y means look up. Controller vertical look is inverted: push the right stick up to look down, or pull back to look up. Pitch limits are -60/+50 degrees. Controller mappings exist for one local player; physical device and duo routing acceptance is separate.

Xbox input uses the additive authored `DA_XboxInput` / `IMC_Xbox` composition. The prior keyboard mappings and original input assets are retained. Both sticks use a 0.18 radial dead zone; right-stick response applies a 1.5 exponent before the existing time-scaled 120-degree/second handler. `StickAimSensitivityScale` is 0.55 only in the new input data asset, leaving mouse input independent. Y calls `CycleWeapon`, which delegates to the same guarded `EquipSlot` path as direct selection. The owning PlayerController observes accepted input-device routing and updates HUD hints only for meaningful input; releases and analog drift do not select a device. This is one-player controller support, with no haptics, aim assist or OS pairing changes. See [CONTROLLER_TESTING](CONTROLLER_TESTING.md) and the current test-matrix entry.

Mac input compatibility: `Config/Mac/MacEngine.ini` sets `Slate.MacUseNewMouseControllerMovement=0` in Engine `[ConsoleVariables]`. The installed 5.8.2 `MacApplication.cpp` implements this switch: false uses AppKit deltas in normal and high-precision capture, while true uses `FAppleMouseController` for captured movement. The setting is read-only and must load before `FMacApplication` is created, so it requires an editor/game restart. This is a targeted workaround for the reported physical mouse freeze after ADS; the precise hardware failure has not been reproduced by automation. ADS itself does not disable look or change input mode. The opt-in `-GunnerLookSmoke` probe verifies the Enhanced Input/controller/camera path across repeated aim/release cycles, but bypasses the native hardware backend and cannot certify this workaround on the user's mouse.

## Movement and camera

The capsule is radius 36/standing half-height 90 cm and crouched half-height 62 cm. Crouch/stand uses native CharacterMovement clearance handling. The portable graph retains genuine idle/forward crouch with stationary crouched ADS. The optional local graph adds actual installed forward/backward/left/right crouch clips and signed actor-relative blend axes for free movement, ADS and crouched high-cover edge steps. Both settings and graph readiness gate this capability. Low cover keeps the separate Quaternius protective base because the taller directional source exceeds the 115 cm barricade during movement.

The armed upper-body layer fades to zero during non-ADS crouch so its upright torso cannot replace the imported protective crouch pose. Standing and crouched ADS restore that layer; sprint also fades it out. This is runtime blending of existing clips, with no manually keyed torso correction. Crouch input rejects an active reload or equip action.

The portable Data Asset retains 380 cm/s travel, 190 aim, 650 sprint and 140 crouch. The local movement profile overrides travel to 300 and sprint to 500 cm/s to suit its hunched rifle gait. Cover travel uses 150 cm/s, or 220 while the context hold requests fast wall travel; actual crouch retains its 140 cm/s limit. Acceleration is 1200 cm/s², braking 1600 cm/s², ordinary rotation 480 degrees/s, jump velocity 440 cm/s and air control 0.2. Sprint heading turns at at most 160 degrees/s while the camera remains free. These are original prototype tuning values, not recovered franchise or Godot numbers.

Normal camera: 320 cm arm, 50 cm shoulder offset and 75-degree FOV. Aim camera: 220 cm arm and 58-degree FOV. Sprint camera: 350 cm arm and 82-degree FOV. Vertical target offset blends from 65 cm standing to 45 cm crouched or 35 cm sprinting; shoulder swaps interpolate. Spring-arm sweeps use a 12 cm probe. These camera changes preserve the original 320 cm normal baseline.

Sprint requires forward intent, grounded standing movement, no aim, no roll and no attached cover. The contextual profile distinguishes a short Space tap from a hold at 0.18 seconds: a press first seeks cover; an open-space tap requests a directional roll on release; hold requests sprint and standing clearance. Sprint searches for cover at 0.08-second intervals. Attached tangent input plus hold requests fast travel, away input detaches, and a repeated press cancels pending entry. J remains a separate jump; E/controller left shoulder requests a roll directly. The portable profile keeps cover/detach/jump fallback on Space. No vault branch is enabled.

Sprint, roll, pending cover entry and falling block combat. Admission reads evaluated Enhanced Input intent and live movement/stance state, avoiding Started/Completed callback-order bugs on simultaneous sprint release and fire. Held ADS resumes after an accepted roll. Melee locks travel; low-cover, active-peek and crouched/pending-crouch melee remain rejected because the source punches are standing full-body actions.

The roll uses `AM_DodgeRoll`, the acquired in-place UAL Roll at a montage rate scale of 1.65. `UGunnerDodgeComponent` selects current travel direction above 30 cm/s, otherwise camera yaw, and validates up to 350 cm of travel. A standing-capsule sweep may shorten the route; less than 100 cm rejects. Floor samples along the route and full destination clearance reject gaps, non-static support and excessive height differences. A native `FRootMotionSource_MoveToForce` drives CharacterMovement over the montage duration; the animation track itself remains in-place. A crouched or attached start is allowed only after full standing-capsule clearance and a supported path pass. Crouch is released and cover detached only after montage/path admission succeeds; a toward-wall rejection preserves cover. Reload, equip and melee reject a roll. Completion, interruption, unpossession, teardown, timeout or leaving walking removes the motion source and releases state. The capsule remains standing-sized throughout. During the roll, the camera arm extends to 350 cm and its target height drops from 65 to 5 cm above capsule center, interpolating back afterward to keep the low pose in view.

Stationary crouch entry/exit presentation is gated by `bCrouchTransitionPoseReady` and passed the focused rendered stance/carry checks. It samples the acquired clips at 2.5x playback only after an actual native stance change in grounded free/high cover. Native CharacterMovement still owns capsule clearance and height. Movement intent, ADS, sprint, weapon handling, dodge, airborne state or an active cover transition cancel presentation; it adds no gameplay movement lock. Low cover bypasses it immediately, including its blend weight, so a tall entry/exit pose cannot replace protected crouch. Fast stance reversal maps progress into the opposite clip instead of restarting from its distant endpoint. An arm-only carry layer continues through free/high-cover crouch and its stance transitions, preserving authored torso, pelvis and leg movement. It yields to ADS/weapon handling; protected low cover bypasses this carry weight and retains its existing branch. Rifle support-hand correction also applies once this carry layer is established.

## Cover geometry and current limits

`UGunnerCoverComponent` queries at most 135 cm, accepts a near-vertical static wall, rejects ankle-height objects using a body-height trace, and classifies low/high using a higher trace. A candidate must pass full-capsule route/destination sweeps and static walkable floor samples no more than 70 cm apart. Admission starts a native `FRootMotionSource_MoveToForce` toward the 52 cm anchor at 300 cm/s, or 450 for a fast approach, bounded to 0.12–0.8 seconds. `IsAttached` stays false during the approach. Every transition tick rechecks the remaining route, wall, support, possession and actual stance. Arrival commits only within 2 cm horizontally and 5 cm vertically; there is no finish teleport. Only the owned motion source is removed on cancellation, preserving unrelated sources. Low cover requests crouch after arrival so native capsule-center changes cannot lift the feet. The plane constraint starts at committed attachment. Stable wall validity normally checks every 0.04 seconds; transitions tick each frame. Detach, unpossession and teardown cancel pending movement and remove the constraint.

Movement projects onto the wall tangent and checks the same height classification, wall and supported floor ahead, including capsule radius and braking distance. Corners require explicit detach/re-attach. Low cover requests crouch while protected and standing for ADS; native standing clearance still applies. High cover has None/SteppingOut/Exposed/Returning states: ADS at the selected open edge requests a 70 cm lateral step, with capsule and supported-floor checks before moving. Existing armed directional locomotion animates the step. Releasing ADS or invalidating its selected side requests return to the stored cover anchor. Normal player movement is suppressed during this transition/exposure; a blocked return times out and detaches at the safe swept position. High-cover peeking may use an actual crouched capsule only when the local directional crouch graph declares readiness; the portable graph remains standing-only. High-cover fire still requires ADS, an allowed edge and the final obstruction traces.

Low-cover blind fire is a separate LMB action without ADS. It preserves the actual crouched capsule and imported crouch torso/head. Native two-bone arm IK places both weapon grips above the measured static barricade top, and the existing additive fire clips supply recoil. The character faces the camera yaw and stops travel while raising/firing; free mouse look remains enabled. The local camera target rises to 90 cm above the crouched capsule center so the aiming ray can see over cover, then returns to its usual offset. RMB cancels blind fire and keeps the existing standing pop-up ADS.

The first blind shot queues until the evaluated pose clears the wall: arm blend >=0.95, firing hand/muzzle >top+4 cm, head below top-5 cm, support hand within 5 cm of its IK target, and view/body facing outward. A 0.9-second unsafe-pose timeout cancels without spending ammo. A brief LMB click queues one shot, the rifle repeats while held, and the pistol remains one shot per press. Releasing after a shot, detaching, unpossessing, losing cover or entering ADS cancels the action. Cancelled input hard-stops pending fire. The blind route checks torso-to-raised-hand and hand-to-muzzle geometry, then traces from the actual muzzle with a 4-degree spread cone; it never ignores the barricade or relocates the shot origin. Normal firing retains its direct torso-to-muzzle obstruction check. This is bounded procedural coverage for low static walls, not a new source animation pack or high-cover blind fire.

The optional profile composes actual Mixamo rifle high-wall idle and lateral poses while standing, idle, grounded and facing outward from the wall. Aim, peek, weapon handling and other actions release that branch. A 25 cm skeletal root inset brings the authored lean toward the wall without changing the 52 cm capsule anchor. Rifle support-hand IK uses the currently evaluated right-hand transform and the weapon grip relation, avoiding a previous-frame weapon-transform dependency. FullBody actions evaluate afterward. Pistol wall poses, dedicated entry/exit actions, braced knee slides, corners, transfers and vaults remain disabled or missing. Fixture heights remain 115/180 cm and the room remains 4400 x 3200 cm; full M3 is outstanding.

## Optional local animation profile

The committed `BP_WardenMotion` retains `ABP_WardenMotionPolish` and its portable settings. At spawn, only a character already configured with motion settings may read `[Gunner.LocalMotion] Profile` from `GGameIni`. The loader accepts only an existing `/Game/Gunner/LicensedLocal/` Data Asset whose AnimationClass derives from `UGunnerAnimInstance` and targets the exact current mesh skeleton. It assigns settings and graph once; nothing loads per frame. Missing/incompatible profiles leave the portable composition active, and `-GunnerIgnoreLocalMotion` explicitly selects that fallback. The foundation pawn has no motion settings and remains unaffected.

Local configuration in `Saved/Config/MacEditor/Game.ini` and `Saved/Config/Mac/Game.ini`, source FBX files, installed plugin copies and their derivatives are ignored by Git. Mixamo FBX sources are preserved in `Saved/LicensedSources/Mixamo`, outside the watched Content tree. Retargeted outputs receive fresh generic empty `UAssetImportData` through native `ClearDerivedAnimationReimportSource`; Python cannot write the UE 5.8 property directly. The helper is restricted to generated local Mixamo target paths, preserving source metadata and report provenance. Guarded authoring preserves recoverable backups; `Tools/enable_local_movement.py` restores the committed character package before enabling local configuration. See [LOCAL_MOVEMENT_SETUP](LOCAL_MOVEMENT_SETUP.md) for the reproduction sequence. This is a development profile, not a cooked distribution or a grant to redistribute raw third-party content.

## Weapon and melee actions

Rifle: 30-round magazine, 240 initial reserve, automatic fire every 0.12 seconds, 20 damage. Pistol: 12-round magazine, 120 reserve, one shot per press with a 0.27-second rate limit, 35 damage. Both default to 10000 cm range. Each weapon retains its own magazine/reserve through swaps. Weapon visuals are hand-attached static template meshes; separately moving magazines/bolts and production weapon effects/audio are not implemented.

A camera trace selects the aim point; the normal-fire body-to-muzzle check rejects a barrel protruding through geometry; a muzzle-to-target trace resolves the actual impact. Only `AGunnerTarget` fixtures receive sandbox damage. A blocked shot still consumes the fired round. Short-lived debug lines/points and a local HUD provide prototype feedback; no per-shot actor spawning is needed.

Fire/equip/reload require a valid animation and grounded authority. Reload commits a conserved magazine/reserve transfer only on uninterrupted montage completion. A serial token rejects stale callbacks, and a timeout clears locks without awarding ammo. Native montage delegates are the current completion contract; no unguarded notify changes ammunition. Equip starts the incoming montage and updates its selected visual after the montage starts successfully.

Reload supports actual/pending crouch and low-cover attachment when the assigned animation graph declares protective reload coverage. The graph layers only the existing reload arm branches onto the genuine crouch pose, keeping the pelvis, torso and head low. Starting reload stops fire/aim; cover peeking is suppressed during reload, and low-cover ADS returns to crouch until completion. Held ADS may resume afterward. The existing completion, cancellation, timeout and ammo-conservation rules apply unchanged. Weapon equip now uses the same protected stance contract through its separate graph capability. It suppresses low-cover peek until completion, then held ADS resumes. The incoming weapon remains selected after an interrupted equip; magazines/reserves are unchanged.

Melee alternates retargeted `Punch_Jab` and `Punch_Cross` full-body montages on accepted presses. A single timer requests impact at 38% of jab duration or 28% of cross duration; code validates authority, possession, current action and active montage before damage. A bounded 135 cm sweep with 35 cm radius, forward-angle gate and occlusion trace applies up to 40 damage to a range target. These are standing punches, not knife combat. Timers/delegates are canceled during interruption, unpossession and teardown.

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

### Jump and empty-trigger presentation

Weapon Data Assets select root-locked takeoff and additive landing montages. `OnJumped` clears conflicting combat and starts takeoff; `Landed` selects recovery for downward impacts above 100 cm/s. CharacterMovement owns trajectory and floor contact. The animation graph fades its upright overlay while airborne so the fall pose remains visible. Recovery does not lock input and yields to combat, crouch, cover, sprint and dodge.

At zero magazine, fire requests a rate-limited dry-fire montage instead of a shot. It has no ammunition, damage or exclusive-action side effect. Actual/pending crouch requires `bCrouchDryFirePoseReady`; its arms compose over genuine crouch. Equip similarly requires `bCrouchEquipPoseReady`. These flags remain false in unprepared graphs. The HUD displays an empty/reload prompt during dry fire.
