# Movement runtime audit

Inspected 2026-09-20 at `6737837`, before the requested complete movement pass. The initial audit below describes that baseline. A bounded follow-up implementation and its later coordinated validation are recorded below. Findings under the initial audit headings are historical, not assertions about the current source. No Unreal instance was launched by this audit worker. The source of the existing behavior is the current Unreal implementation, not the historical Godot documents.

The user's request expands movement beyond the previous sandbox. Character appearance, enemy AI, waves and multiplayer remain separate. Gears of War 2 research should determine the requested interaction vocabulary; this document describes how to implement an original Gunner movement system safely. Exact franchise timing, undocumented exploit behavior and animation provenance cannot be established by this source audit.

## Current implementation and focused acceptance

The later implementation adds mutation-free cover preflight and a swept, supported native approach; safe cancellation and braking-distance wall bounds; contextual tap-roll/hold-sprint/fast-wall input; a bounded sprint travel heading with free camera look; rolls from clear crouch/cover starts; live combat admission; and held ADS recovery after roll. The local graph adds actual directional free/high-cover crouch, hunched rifle sprint, rifle high-wall idle/lateral motion and evaluated support-grip correction. The portable Blueprint remains unchanged; ignored local config supplies licensed animation content.

An earlier profile completed 24 traversal scenarios in each of two rendered PIE sessions with zero failures/skips, plus a commandlet asset gate and wall/sprint visual inspection. A later normal-editor restart auto-reimported 23 Mixamo packages and corrupted the target poses; this invalidates those results as final acceptance. The affected tree is preserved and recovered with FBX outside Content and fresh generic empty import data on targets. A fresh persistence gate passed 2,013 poses across eleven clips and fourteen rig/target hashes. The local graph also adds stationary crouch entry/exit and arm-only carry preserving the authored torso/legs. The recovered 30-scenario traversal (612 checks), movement (186), blind-fire (68), crouched-reload (224) and polish (242) runs pass. Portable fallback also passed 184 checks; post-normal-editor persistence revalidated all 2,013 poses and fourteen package hashes with zero errors/warnings. Final acceptance and limitations are recorded in [TEST_MATRIX](../TEST_MATRIX.md). Braced knee slides, corners/transfers, attached vaults, pistol wall poses, full directional roll variants and knife handling remain disabled or missing.

## Existing foundation worth retaining (initial audit)

`AGunnerCharacter` owns an ordinary `ACharacter` and `UCharacterMovementComponent`. Keyboard/stick movement, native crouch clearance, spring-arm collision, independent mouse look, shoulder swap and grounded animation state are already present. The owner-specific Enhanced Input context is removed without clearing unrelated contexts. Completed and Canceled releases exist for movement, aim, fire, sprint and jump.

`UGunnerCombatComponent` conserves per-weapon ammunition, guards authority and montage completion, rejects stale completion callbacks, and cancels pending damage/ammo commits. Low-cover blind fire checks the evaluated hands, head, grip and real muzzle before releasing ammunition. Protective reload, equip and dry-fire arm layers preserve the actual crouch torso. These behaviors should remain acceptance requirements during movement changes.

`UGunnerDodgeComponent` already provides a useful model for short, bounded movement actions: preflight the standing capsule and supported route, play a licensed in-place montage, apply a native `FRootMotionSource_MoveToForce`, and remove only the owned source on completion/interruption. It validates montage duration and cancels if the character stops walking. It does not provide directional roll variants, root-motion vaulting or a crouched collision profile.

`UGunnerCoverComponent` already stores a weak wall component, normal and low/high classification; constrains attached movement to a wall plane; validates the wall periodically; and drives high-cover edge exposure using actual CharacterMovement input. Its current scope is a static flat wall segment, with explicit detach at corners.

## Main gaps and risks (initial audit)

| Finding | Evidence in the inspected source | Consequence / proposed correction |
|---|---|---|
| Entry is an instantaneous snap | `GunnerCoverComponent.cpp`, `TryAttach`, calls `SetActorLocation(Desired, true, &Sweep)` before attachment | Replace with a candidate query that does not mutate the pawn, then a short swept CharacterMovement transition. Do not call the current snap repeatedly as an interpolation mechanism. |
| Failed entry may still move the pawn | `TryAttach` returns false when the swept move stops more than 3 cm short, retaining the partial position | A rejected query should cause no motion. A transition that was accepted and later blocked should stop at its safe swept position and release its locks. Distinguish the two outcomes. |
| Entry has no destination/route floor preflight | `TryAttach` checks starting grounded state and a horizontal wall trace, but no support at the destination | Apply the roll/peek approach: walkable static support, bounded height change, route support and capsule clearance before action admission. Never pull a character onto a ledge merely because a wall is reachable. |
| Attached travel only looks for a wall ahead | `ConstrainMovement` projects onto a tangent and calls `WallAt` 44 cm ahead | A wall can continue beside a drop. Validate walkable support along intended travel and stop before unsupported edges. Runtime floor loss still detaches. |
| Cover height is cached once | `bLow` is assigned only in `TryAttach` | A single mesh with changing top height may retain a stale protected stance. Revalidate the usable cover band at candidate anchors, or explicitly reject segments whose height class changes until a guarded stance transition is implemented. |
| Cover entry has no explicit state | Public state is `bAttached`; only peeking has a substate | Add Entering/Attached/Leaving/Transferring states, with an explicit transition query used by combat, input and animation. Do not make all callers infer transition ownership from montage playback. |
| Sprint travel is not constrained by sprint turning | `Move` uses camera-relative forward plus full lateral input; actor rotation is independently oriented to movement | A heavy sprint needs a travel heading with an authored angular limit and lateral steering behavior. Merely lowering `RotationRate` leaves velocity free to cut sideways under a lagging body. |
| Sprint presentation has limited posture/camera treatment | Current sprint plays `A_UAL_Sprint_Loop`, changes speed to 650 and arm/FOV to 350/82 | Keep the genuine clip, but add selected camera height, easing and heading/acceleration tuning. A low roadie posture needs matching licensed source coverage; camera lowering alone does not create the motion. |
| Movement/combat admission uses some last-frame state | Sprint, cover fire blockers and `bCombatBlocked` are derived in Character Tick; input actions can run before that update | Validate current action state and intent at admission. Compute one aggregate combat block reason; a completing component must not clear a block owned by another system. Add same-frame action-order probes. |
| Context traversal is only detach / attach / jump | `AGunnerCharacter::Traverse` has no corner, transfer, mantle or slip branches | Introduce a deterministic contextual decision, preserving a separate explicit jump action if desired. Have the version-specific research settle the default button vocabulary before presenting it as Gears 2 parity. |
| Cover edge behavior only exposes a standing pawn | `FindPeekDestination` explicitly rejects crouch/pending crouch and moves 70 cm sideways | Preserve this honest limitation until directional crouch/edge source clips exist. A transition framework must not silently enable an upright pose on a crouched capsule. |
| No dedicated cover source animation set | Animation pipeline and source inventory have crouch forward/idle, sprint, jab/cross, roll and weapon clips | Treat wall idle, low/high lateral motion, entry slide, corner transition, transfer and vault as separate readiness capabilities. Never enable them through one generic `bCoverReady` flag. |
| Camera tuning is mostly hardcoded | Character Tick uses literal arm, FOV, shoulder, target-height and interpolation values | Move coherent movement/camera tuning into the existing motion Data Asset. Keep the normal 320 cm baseline and document action-specific changes. Avoid a new global camera manager. |

These are source-observed limitations or risks. Same-frame input admission and unusual geometry cases require a rendered probe before being reported as reproduced user bugs.

## Concrete state and ownership design

Keep the existing domain components. Extend cover instead of creating a universal gameplay state machine. A small explicit cover state can use `Detached`, `Entering`, `Attached`, `Peeking`, `Returning`, `Cornering`, `Transferring` and `Leaving`; add `Vaulting` only when its actual montage and route policy are integrated. Whether peeking remains a nested enum is an implementation choice, but there must be one authoritative owner of each transition.

Expose distinct questions:

- `HasCoverAnchor`: an existing valid protected anchor is retained, including during a planned movement away from it.
- `IsAttached`: the pawn has committed to a stable protected wall position.
- `IsTransitioning`: a cover action owns travel/rotation; ordinary movement, new dodge and incompatible weapon actions are blocked.
- `CanAim`, `CanFire`, `CanReload`, `CanEquip`: derive from actual stance, transition, montage capability and combat state, not merely whether a wall pointer exists.

Store a candidate separately from the committed anchor. A candidate should hold weak wall/support references, surface normal/tangent, standing or crouched stance, feet location, target capsule transform, measured cover top, selected side, action kind and rejection reason. Candidate queries must not change position, action state, montage, capsule or constraints.

For an admitted movement action, store a monotonic serial, source and destination anchors, movement source ID, active montage, expected duration, elapsed time and previous rotation/constraint settings. A single cleanup path invalidates the serial first, clears the owned timer/delegate/source, stops or releases presentation, and restores a safe locomotion state. Completion commits the destination only when the capsule is close enough, support and wall remain valid, and the action serial matches.

Use the existing CharacterMovement root-motion-source pattern for short translations whose animation is truly in-place. Installed UE 5.8 provides `FRootMotionSource_MoveToForce`, `PathOffsetCurve`, `bRestrictSpeedToExpected`, source-ID removal and finish-velocity policy in `Engine/Source/Runtime/Engine/Classes/GameFramework/RootMotionSource.h`. This is an available engine mechanism, not evidence that a new transition has been tested. A vault may need a custom CharacterMovement mode or montage root motion with warping; its collision arc and falling behavior must be validated before selecting that implementation.

Only one traversal source should own the pawn at once. Character-level admission can arbitrate cover transition versus dodge, while each component retains its own cleanup. Avoid independent components toggling a shared boolean blocker off. Have Character recompute aggregate restrictions from live state, or query those states directly from `CanStartAction`.

## Geometry and stance invariants

The standing capsule remains radius 36/half-height 90 cm; crouched half-height remains 62 cm. Attached offset remains 52 cm, query reach 135 cm and fixture heights 115/180 cm unless a documented design change is intentional.

1. Construct destination center from the measured feet and required stance half-height. Native crouch moves center Z by 28 cm; a standing-center target used after crouching can lift the character. Wait for actual crouch or use stance-correct target geometry before applying the source.
2. Validate start overlap, full capsule route, destination overlap, walkable supported path, allowable height change and target wall orientation. A visibility line to a wall alone is insufficient.
3. Keep the old wall plane during stable movement. For an accepted corner/transfer, disable it only after preflight succeeds. Commit the new plane and origin only after successful arrival. Failed admission leaves the old anchor untouched.
4. Revalidate weak component references and geometry during travel. New obstacles, disappearing support, wall destruction, lost possession, falling and montage interruption must stop motion safely without teleporting to either endpoint.
5. Never snap a blocked transition to its destination at timeout. Release or restore a valid nearby anchor at the actual swept position.
6. Low-cover transitions must keep the genuine crouched base pose and its evaluated head below the measured cover top when they claim protection. Treat entry/exposure time as exposed unless the geometry and evaluated pose prove otherwise.
7. Keep high-cover corner movement, cover-to-cover transfer and through-vault separate. A corner path goes around exterior geometry; a transfer crosses an exposed gap; a vault crosses the barrier. Different paths, poses and failure behavior are required.
8. Constrain fast cover travel before the capsule leaves supported cover. Raising movement speed magnifies any fixed-distance edge lookahead defect; probe at least the braking/lookahead distance needed by the configured speed.

## Input and camera policy

Separate held intent from accepted actions. Press/hold/release timing should choose one context action, not independently launch a roll, attach and sprint. An intentional short input buffer may retain one recent traversal request during a recovery window; bind it to a deadline and consume it exactly once. Do not queue unbounded traversal chains or weapon commits.

Retain Completed and Canceled handling for every held action. For focus loss, controller disconnect, context removal and unpossession, clear held aim/sprint/traverse and pending traversal requests, stop pending fire and remove owned motion. Test actual viewport focus/capture separately from synthetic controller injection because the historical Mac mouse issue was below the Enhanced Input layer.

For sprint, maintain a travel heading driven by input and limited turn rate; derive both displacement and body presentation from it. Mouse look can remain freely movable while the heading catches up. Use acceleration/braking appropriate to the source gait and blend start/stop instead of making the body orientation merely lag a freely strafing velocity vector.

A camera profile should cover normal, aim, crouch, sprint, cover transition and roll, with action-specific target height, arm, shoulder offset, FOV and blend rate. Keep camera collision active in every state and avoid forced camera yaw snapping during a slide. Camera bob/shake should be subtle and separately tunable; it cannot supply missing posture animation. Do not introduce motion blur to imitate speed, given the art direction and 16 GB development baseline.

## Animation capabilities and truthful enablement

| Requested family | Existing useful motion | Admission gate for the complete behavior |
|---|---|---|
| Weighted armed walk/jog/aim | Epic rifle/pistol eight-direction locomotion and Aim Offsets | Validate blend speeds, all diagonals, stop/start and both shoulders; foot-plant/turn assets remain separate polish coverage |
| Low sprint / roadie-like locomotion | Genuine UAL sprint loop | Verify actual posture and weapon-hand alignment; an ordinary sprint cannot be labeled an acquired low sprint |
| Directional evasive roll | One genuine forward UAL roll, reoriented to travel | Explicitly document reorientation; no backward/left/right authored roll variants currently exist |
| Smooth approach to cover | Existing standing/crouch movement may supply an honest approach | A dedicated slide claim requires a genuine slide clip; normal gait interpolation is not a slide animation |
| High/low wall idle and lateral cover motion | Generic armed and forward crouch clips | Licensed wall-contact/directional coverage and a graph capability per stance |
| Corner transition and edge slip | Current standing directional step-out | New corner/exit poses, checked route and interruption behavior; do not relabel the existing 70 cm ADS step |
| Cover-to-cover transfer | No dedicated source in the accepted composition | Correct acquired transition clip, landing stance and path validation |
| Low-cover mantle / through-vault | No enabled through-vault clip | Correct acquired source, contact/clearance samples and full path/landing acceptance; a climb-up clip is not interchangeable |
| Crouched directional ADS | Only forward crouch locomotion | Keep disabled until genuine directional crouch coverage and armed overlay are proven |

Source acquisition and import should be done in parallel with the native transition framework. Missing source assets must remain visible in the coverage ledger; importing a pack does not by itself complete the action.

## Acceptance cases with high value

Add a focused movement probe and keep existing motion/blind-fire/crouched-reload/polish probes as regressions. Use transient geometry in the range or a separate movement fixture map; preserve the foundation map.

| Case | Required observation |
|---|---|
| Entry distance sweep | Near, middle and maximum valid queries enter smoothly; just-outside range rejects without position change; no one-frame position jump at admission |
| Diagonal entry and two visible walls | Deterministic target choice; intended surface normal; no clipping through the intervening wall |
| Rejected entry | An obstructed destination, unsupported ledge, wrong-height obstacle or unavailable montage changes neither position nor action state |
| Obstacle introduced mid-entry | Swept travel stops safely; montage/source/locks clean up; subsequent aim, move and attach work |
| Low-cover entry | Real crouch becomes active; center/feet stay grounded; evaluated head stays below the top once protected; no vertical lift from a stale standing target |
| High/low transitions on one mesh | Height class is revalidated or the move rejects predictably; protection is never claimed at a short section |
| Fast cover travel and edge stop | Both directions, both weapons and low/high cover stop before unsupported ends; route support is checked at configured top speed |
| Corner / transfer / vault | Each has distinct positive and negative fixtures, correct source motion, full capsule path, landing support and endpoint orientation |
| Return path obstruction | ADS release or canceled transition never forces through a new obstacle; safe detach leaves controls and camera responsive |
| Sprint steering | Mouse look remains live; large yaw changes produce the selected turn radius; lateral input cannot create a body/velocity mismatch; release brakes predictably |
| Sprint and weapon action ordering | Same-frame press order permutations of sprint, fire, reload, ADS and traversal never grant a forbidden shot or ammo transfer |
| Interruption matrix | Montage stop, unpossession, falling, context removal, timeout and target destruction remove only the owned source and all stale callbacks |
| Repeated context action | Hold/tap/double tap, rejected input and buffered input each consume no more than one request; repeated release does not toggle extra actions |
| Physical input | Real mouse capture survives repeated aim, sprint and cover transitions; actual gamepad acceptance is documented separately if no controller is available |
| Camera collision | Both shoulders and all stances against a wall, inside corner and low ceiling keep the view outside geometry and recover smoothly |
| Frame-rate variation | Repeat transitions with different frame limits and a frame hitch; endpoint tolerance, cleanup and single-commit behavior remain stable |
| Existing protected combat | Low-cover rifle/pistol blind fire, reload, equip and dry fire retain all evaluated head/grip/muzzle and ammo invariants after every new traversal action |

For each implemented family, record both its native state/geometry assertions and actual rendered animation evidence. A passing collision test with a wrong pose is a failure of the complete movement goal. A visually attractive motion with stale input, unsupported landing or uncontrolled ammunition side effects is also a failure.

## Suggested implementation sequence

First add transition state, pure candidate queries, cleanup ownership and a smooth checked entry using truthful existing motion. In the same pass make contextual input and sprint heading/camera tunable and test rapid interruptions. Acquire and retarget the missing cover/directional/slide/vault source coverage in parallel.

Then integrate one fully validated movement family at a time: directional protected cover locomotion and slide entry; corner/edge exits; cover-to-cover transfer; low-cover vault. Prove each family's animation, route and cancellation before enabling its input branch. Finish with the combination matrix, continuous play, frame-rate/camera stress and physical input. Update the motion milestone around the actual complete coverage, leaving the horde-game systems for the next subsystem.

## Bounded implementation follow-up

The coordinating task subsequently assigned only `GunnerCoverComponent.h/.cpp` to this worker. Those files now implement a checked native approach while the coordinating task owns Character/input/camera/combat integration.

- `TryAttach(SearchDirection, bFastApproach = false)` returns whether a transition was accepted. `IsAttached` remains false until actual arrival. `IsTransitioning`, `IsEnteringLowCover`, `CancelTransition` and the pending value from `GetNormal` expose explicit integration state.
- Candidate queries are mutation-free. They require ground, controller/authority, settled stance, no competing movement source, same static wall at body height, consistent low/high classification, full capsule sweeps/clearance, and static walkable support at no more than 70 cm intervals including the final floor.
- Accepted entry uses `FRootMotionSource_MoveToForce` at 300 cm/s or 450 cm/s for the fast request, with a bounded duration. It retains the current actual capsule until arrival, avoiding a standing-to-crouched center-height mismatch. Character is responsible for requesting low-cover crouch after attachment commits.
- Each approach tick checks possession, walking, capsule height, the destination wall/height and the remaining swept, supported route. Revalidating the entire remaining route prevents CharacterMovement from sliding around a newly introduced obstruction and later committing an unintended path. Completion requires the real swept position within 2 cm horizontally and 5 cm vertically. No completion teleport occurs. Timeout or invalidation removes only the owned motion source, clears input velocity and releases pending state at the actual position. Detach and EndPlay cancel a pending approach.
- Attached movement checks same wall, same low/high class and supported floor ahead. Its lookahead includes capsule radius and the configured braking distance, sampled at no more than 70 cm intervals. A height-class change cancels attachment rather than silently reusing stale protection state.
- `CanPeek` and `ConstrainMovement` explicitly block a pending entry. Character must also block jump/dodge/new combat and handle repeated context presses while entering. No dedicated slide, wall lean, corner, transfer or vault source motion is claimed by this approach.

`git diff --check` passed for the owned files. No build or gameplay pass is claimed by this worker; the coordinating task serializes builds, rendered probes and visual review. Existing attachment probes must allow the short approach interval instead of assuming `TryAttach` means an instantaneous committed anchor.

Follow-up integration allows the existing physical high-cover edge step from settled crouch only when the assigned `UGunnerAnimInstance` declares `bDirectionalCrouchPoseReady`. Its sweep and floor validation use the actual crouched capsule. This extends the source-coverage gate; it is not a dedicated lean/corner clip.

`AGunnerTraversalProbe` and `Tools/validate_traversal_editor.py` initially defined 22 transient-fixture scenarios and now covers 30; the recovered 30-scenario suite passed both rendered sessions for two rendered PIE sessions. Fixtures sit on a temporary floor at Z=400 cm so a missing support test cannot silently hit the preserved foundation floor. Coverage includes normal/fast smooth entry, low-cover arrival, already-crouched entry, side obstruction and unsupported-destination rejection, geometry introduced after admission, explicit cancel, duplicate requests, held/repressed context input, target destruction, changed cover height, preservation of an unrelated native movement source, high/low cover roll-away, toward-wall roll rejection, and low-ceiling crouched-roll rejection. Screenshots capture actual approach, attachment and roll presentation. The root task registers and runs the opt-in `-GunnerTraversalSmoke` actor; no probe is part of normal play.


The later scenarios require the installed directional crouch capability. They exercise rifle/pistol forward, backward and lateral crouched ADS using actual local velocity and the evaluated graph axes; a crouched high-cover edge step with measured head height; retained protective low-cover locomotion; and contextual tap-roll/hold-sprint, limited sprint heading, mouse yaw/pitch and immediate release-to-fire. Missing capabilities are explicitly logged as skipped and become failures under `-GunnerRequireTraversalCapabilities`. Tests also exercise first-input admission immediately after approach completion/cancel and reject free crouch hip fire immediately after detaching low cover. These additions are not a claim of a new runtime pass.

The first coordinating-task run of the 16-scenario build reported the high/low cover roll cleanup checks one tick before native source removal had settled. The probe now records feet, movement mode, active sources and plane constraints and allows no more than 0.1 seconds after montage completion for cleanup. It still requires grounded feet, no owned/stale movement and no old wall constraint; this change does not waive any final-state invariant. The coordinating task owns the final rebuilt results.

Live combat admission now queries actual cover/stance plus current Enhanced Input aim intent. Cached restriction flags still interrupt an already active action, but they cannot allow an immediate hip shot after leaving protected crouch or incorrectly reject the first action after traversal releases. Empty-trigger feedback retains its established ordering before the fire restriction. Installed Enhanced Input evaluates action values before dispatching delegates, while Started delegates precede Completed delegates: this is why the Character's live aim, movement and sprint queries are needed for simultaneous sprint release and fire press.

## Installed vault source geometry proposal — not enabled

The installed source report `Saved/installed_traversal_inventory.json` identifies `/MoverExamples/Characters/Mannequins/Animations/Manny/VaultOver.VaultOver` as a 2.033333-second vault with 390.09 cm of forward root displacement and a 123.27 cm root-height peak. The source forward direction is +Y and the provisional mesh's -90-degree relative yaw maps it to actor +X. This is real source inventory evidence; it is not a retargeted gameplay or contact-quality acceptance result.

The source geometry differs materially from the existing 52 cm cover attachment. Its right hand is approximately 151.8 cm forward of the starting root and 127.6 cm above the starting floor during the early plant, around 0.51–0.61 seconds. At a 130 cm approach distance, this places the hand about 22 cm across a 40 cm-deep barrier. A 115 cm barrier would still need roughly 12.6 cm of contact correction and actual rendered hand/weapon validation. Beginning the entire clip at the current attached anchor would drive the early capsule into the wall. Do not teleport backward or skip the launch phase to disguise that mismatch.

An offline 2D capsule-versus-rectangle check of the report's 61 root samples used the full standing capsule (36 cm radius, 90 cm half-height) and a 2 cm margin. A 115 cm-high, 40 cm-deep rectangular barrier had a viable sampled launch-distance interval of approximately 124–199 cm. Increasing thickness to 100 cm narrowed that interval to approximately 124–139 cm. A 125 cm barrier had no viable sample in this check. This only illustrates the geometry: Unreal must validate densely sampled sweeps, floor support, full 3D collision, retargeted pose and hand contact before enabling any action.

A bounded implementation could admit this as an open approach vault around 130 cm from the 115 cm barrier, preserve the source's roughly 3.9 m total travel, and require the entire far landing to be supported. A project-owned root-locked source can drive presentation while a native root-motion source follows its measured forward/vertical arc; a sampled path-offset curve can express deviation from a straight endpoint interpolation. The vertical phase needs an explicit movement-mode and interruption policy. Every arc segment must pass a full-capsule sweep before admission and during travel, and completion must return to grounded walking only on a validated landing. No compressed short vault, attached vault, contact pose or weapon stow behavior is currently certified by this proposal. Implementation remains unassigned and disabled.

Character Tick and cover decisions now use the same evaluated aim intent as combat admission. Holding RMB through a dodge resumes ADS after the movement lock ends; no new aim or fire press is required. Sprint and traversal hold decisions in Tick also use evaluated action values, while the cached traversal press remains the lifecycle token for release-to-roll. Scenario 21 checks actual roll suppression, continuous held-RMB resumption in gameplay and the animation graph, and the subsequent aim release. The recovered 30-scenario run passed this held-aim case in both rendered sessions; final broader acceptance and persistence are recorded in TEST_MATRIX.
