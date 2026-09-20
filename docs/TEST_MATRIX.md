# Crouched reload acceptance — 2026-09-19

Rifle and pistol reload now retain the real crouch torso, head and legs while the existing Epic reload montage drives the arms. `ABP_WardenCrouchReload` composes the protective arm layer; the old graph is preserved and source animation tracks are unchanged. This supersedes the earlier standing-only reload limitation. Weapon switching still requires standing outside low cover. The HUD's stale reload hint and overwritten cover status were corrected.

| Gate | Actual result / evidence |
|---|---|
| Editor C++ build | Succeeded with the focused probe and scoped input guard. `evidence/crouch-reload/build-editor.txt`. |
| Asset authoring | `Tools/install_crouch_reload.py` completed with **0 errors / 0 warnings**. The persisted assignment is `ABP_WardenCrouchReload`; both blind-fire and crouch-reload readiness flags are true. `authoring.txt`, `authoring-report.json` under `evidence/crouch-reload/`. |
| Focused rendered acceptance | Two PIE sessions, **112 checks each / 224 passes / zero failures**; both native summaries and the two-cycle driver report zero failures. [Focused log](evidence/crouch-reload/editor-smoke.txt). |
| Context and real pose | Rifle/pistol each tested in free crouch and attached low cover. The selected montage advances through the active `UpperBody` slot; evaluated left-hand travel is **29.22–82.30 cm** in mesh space after blend-in, so actor movement cannot satisfy the motion check. |
| Protection and stance | Sampled maximum head heights are **88.22–88.77 cm** against measured **115 cm** cover. The crouched capsule half-height remains 62 cm. Samples cover the active reload after a **0.35-second ADS-to-crouch transition allowance**; these measurements do not certify the initial handoff as fully hidden. Crouch toggling is blocked during reload and works again afterward. |
| Ammunition and interruption | Ammo changes once, on uninterrupted completion, with magazine-plus-reserve conserved. Deliberately interrupting the actual montage transfers no ammo, including after its former completion/timeout window. Full-magazine reload is a no-op. |
| Layer and action handoffs | A graph with readiness disabled rejects crouched reload. Blind-fire-to-reload clears arm IK so it cannot pin the handling pose. Held low-cover ADS returns to crouch while reloading, then resumes standing ADS after completion. Movement and aim remain available afterward. |
| Rendered evidence | Eight untouched early/mid captures: [rifle free early](evidence/crouch-reload/crouch_reload_rifle_free_early.png), [mid](evidence/crouch-reload/crouch_reload_rifle_free_mid.png); [rifle cover early](evidence/crouch-reload/crouch_reload_rifle_cover_early.png), [mid](evidence/crouch-reload/crouch_reload_rifle_cover_mid.png); [pistol free early](evidence/crouch-reload/crouch_reload_pistol_free_early.png), [mid](evidence/crouch-reload/crouch_reload_pistol_free_mid.png); [pistol cover early](evidence/crouch-reload/crouch_reload_pistol_cover_early.png), [mid](evidence/crouch-reload/crouch_reload_pistol_cover_mid.png). The second session replaces the first session's image filenames; both sessions retain separate log records. |
| Existing motion regression | Two rendered PIE sessions, **92 checks each / 184 passes / zero failures**. Standing reload, aim, movement, cover return, obstruction, melee, dodge and possession remain passing. `evidence/crouch-reload/motion-regression.txt`. |
| Blind-fire regression | Two rendered PIE sessions, **34 checks each / 68 passes / zero failures**. Both weapons retain actual muzzle clearance, protected head height, obstruction checks and cancellation. `evidence/crouch-reload/blind-fire-regression.txt`. |
| Game C++ build | Succeeded, no project compiler diagnostics. `evidence/crouch-reload/build-game.txt`; this is not a cooked package test. |

Run one Unreal process at a time:

```bash
./Tools/build_macos.sh GunnerEditor
./Tools/open_editor_macos.sh -GunnerCrouchReloadSmoke -nosound \
  "-ExecCmds=py $(pwd)/Tools/validate_crouch_reload_editor.py" "-LogCmds=LogPython Log" -stdout
./Tools/open_editor_macos.sh -GunnerMotionSmoke -nosound \
  "-ExecCmds=py $(pwd)/Tools/validate_motion_editor.py" "-LogCmds=LogPython Log" -stdout
./Tools/open_editor_macos.sh -GunnerBlindFireSmoke -nosound \
  "-ExecCmds=py $(pwd)/Tools/validate_blind_fire_editor.py" "-LogCmds=LogPython Log" -stdout
./Tools/build_macos.sh Gunner
```

The focused implementation is `Source/Gunner/Private/Tests/GunnerCrouchReloadProbe.{h,cpp}` with `Tools/validate_crouch_reload_editor.py`. Raw accepted inputs are `Saved/crouch-reload-pie-final-native.log`, `Saved/crouch-reload-authoring-native.log`, and `Saved/crouch_reload_authoring_report.json`. The authoring report records unchanged Epic rifle/pistol reload sources, their existing montages and `UpperBody` slots, 2.2/2.0-second source lengths, the assigned graph and recoverable asset backup. Authoring is already applied and is not an ordinary launch step.

The initial focused run is **not acceptance**: its first cycle passed, but its second recorded 13 failures beginning with an interrupted reload. That log also contains two `DebugManager.CycleToPreviousColumn` commands, bound by the installed engine to LeftShift, which this probe never injects. The synthetic tests now temporarily ignore viewport-originated physical input through `FGunnerProbeInputGuard`, flush existing key state once before injection, and restore the prior viewport setting during cleanup/EndPlay. Direct `PlayerController::InputKey` still enters Enhanced Input. This isolates test input without changing normal gameplay. The guarded rerun passed both cycles; observational montage-end and ammo diagnostics remain in the probe.

All three final probe runs passed across six PIE sessions: **476 live checks, zero failures**. No project compile, Blueprint, Python, asset-load or gameplay errors appeared in those runs. Engine warnings remain `r.MotionVectorSimulation` render-thread access and an outstanding analytics HTTP request during shutdown. The Game build repeats the installed `MetalShaderConverter/include/metal_irconverter_ext` missing-directory warning. Physical mouse/controller delivery, audio, cooked packages, Windows, multiplayer, player damage/invulnerability and production animation polish are outside this acceptance.

---

# Crouched low-cover blind fire — 2026-09-19

Implemented LMB blind fire without ADS while attached to low static cover, using the real crouch pose, arm-only native IK and the existing Epic additive fire clips. RMB retains standing pop-up ADS. The old graph remains available; no source motion tracks were modified.

| Gate | Actual result / evidence |
|---|---|
| Editor and Game builds | Both succeeded, no project diagnostics. `evidence/blind-fire/build-editor.txt`, `build-game.txt`. Game build is not a cooked package test. |
| Existing motion/combat regression | Two further rendered PIE sessions: **184 passes, zero failures**. Standing/crouched ADS, reload/equip, cover exposure/return, shot obstruction, melee, roll and possession lifecycle remain passing. `evidence/blind-fire/motion-regression.txt`. |
| Blind-fire live checks | Two rendered PIE sessions, **34 checks each / 68 passes / zero failures**. `evidence/blind-fire/editor-smoke.txt`. |
| Evaluated protection and clearance | Against measured 115 cm cover, head 88.05–88.36 cm; rifle hand 124.83 cm / muzzle 135.10 cm, pistol hand 125.05 cm / muzzle 134.33 cm. Actual crouch/capsule remains 62 cm; upright torso overlay stays below 0.05 weight. |
| Weapon contact | Actual support-hand target errors: rifle 4.55 cm during recoil, pistol 1.68 cm. Both pass the <=5 cm firing guard without bone stretching. This is provisional contact, not production-quality hand polish. |
| Rifle and pistol | Rifle repeats while held; pistol fires once per press; quick clicks queue one raised shot; actual target beyond the wall takes damage. Release stops repeat fire and lowers arms. |
| Lifecycle and geometry | No ammo spent while raising or looking away. Movement blocked during blind fire. Detach, unpossess and switching to ADS cancel pending shots. Actual muzzle trace cannot damage through an intervening wall. |
| Existing ADS | RMB still exposes the standing low-cover pose, fires normally and restores crouch on release. |
| Asset persistence | Fresh process confirms `ABP_WardenBlindFire` is assigned and ready. Character assignment repaired after initial compile/save order discarded it; original packages backed up. `asset-assignment.txt`, authoring and repair JSON reports. |
| Visual inspection | Untouched [rifle blind fire](evidence/blind-fire/blind_rifle_fire.png), [pistol blind fire](evidence/blind-fire/blind_pistol_fire.png) and [release](evidence/blind-fire/blind_rifle_released.png) inspected from the real gameplay camera. Both hands/weapon raise while torso/head remain hidden; release returns to the acquired protective crouch. |

Reproduce with one Unreal process at a time:

```bash
./Tools/build_macos.sh GunnerEditor
./Tools/build_macos.sh Gunner
./Tools/open_editor_macos.sh -GunnerBlindFireSmoke -nosound \
  "-ExecCmds=py $(pwd)/Tools/validate_blind_fire_editor.py" "-LogCmds=LogPython Log" -stdout
```

Require both `GUNNER_BLIND_COMPLETE failures=0` native summaries and `GUNNER_BLIND_PIE_COMPLETE cycles=2`; driver completion alone is insufficient. The test adds transient broad targets and a blocking wall, injects ordinary gameplay input, measures evaluated hands/head/muzzle and target damage, and restores background throttling before exit. It does not certify physical controller hardware or native macOS mouse delivery.

The accepted blind-fire run contains no project compile, Blueprint, Python, asset-load or gameplay errors. Its engine warnings are the existing `r.MotionVectorSimulation` render-thread access and an outstanding analytics HTTP request during shutdown. The Game build repeats the installed `MetalShaderConverter/include/metal_irconverter_ext` missing-directory warning. Initial failed runs are excluded: the first loaded the previous graph because the changed component assignment was not persisted; the next found 11.42 cm rifle support-hand error. The installer now compiles before setting/force-saving the component assignment, and the rifle grip target moved rearward/closer without relaxing the reach gate. The broader motion run also found a test timing issue: screenshot capture delayed the high-cover return completion flag until 1.024–1.040 seconds, with the first cycle already within 1.9 cm of its anchor at 1.007 seconds. That assertion now waits for the same state/position conditions up to 2.6 seconds, immediately fails a detach, and logs actual completion. The runtime return logic and its 2.5-second failure timeout are unchanged.

Scope limits: low static cover with a reachable flat top; stationary procedural arms, 4-degree shot spread, no high-cover blind fire or dedicated source cover clips. This is not a new damage/invulnerability system; protection is the visible crouch/geometry already present in the sandbox. Audio, cooked builds, Windows, multiplayer and performance budgets remain untested. The explicit inventory of useful available-but-unused clips is in [ANIMATION_BACKLOG](ANIMATION_BACKLOG.md).

---

# ADS mouse-look compatibility — 2026-09-19

The user reported physical mouse look remaining stuck after releasing RMB on macOS. No gameplay ADS branch suppresses look input. The project now selects Unreal's AppKit path with `Slate.MacUseNewMouseControllerMovement=0` in `Config/Mac/MacEngine.ini`; the default captured-input path uses the Apple mouse-controller bridge. Installed engine source supports this configuration and its startup timing. This is a candidate compatibility fix, not a hardware failure proven by synthetic tests. **Physical-mouse confirmation remains pending.**

| Check | Actual result / evidence |
|---|---|
| Editor and Game builds | Both succeeded, no project compiler diagnostics. `evidence/look/build-editor.txt`, `build-game.txt`. |
| Mac startup configuration | Native startup log sets the CVar to 0; the Python runtime getter also reports 0. `evidence/look/editor-smoke.txt`. |
| Repeated ADS look | Two rendered PIE sessions, 98 checks each: **196 passes, zero failures**. Rifle aim/release/repeat, pistol aim/release, crouched aim/release, low-cover aim/release and detach. |
| Controller and view | Signed synthetic MouseX/MouseY inputs rotate yaw/pitch in the expected directions, the live camera follows control rotation, look input remains enabled and the cursor remains hidden. |
| Visual inspection | Untouched [rifle ADS](evidence/look/look_rifle_ads.png), [released ADS](evidence/look/look_rifle_released.png) and [released low-cover ADS](evidence/look/look_cover_released.png) frames inspected. Normal camera distance and protective crouch return after release. |
| Hardware verification | Normal editor restarted with CVar 0 and no smoke flags; the user was asked to test their physical mouse. Awaiting confirmation. |

Reproduce with one Unreal process at a time:

```bash
./Tools/build_macos.sh GunnerEditor
./Tools/build_macos.sh Gunner
./Tools/open_editor_macos.sh -GunnerLookSmoke -nosound \
  "-ExecCmds=py $(pwd)/Tools/validate_look_editor.py" "-LogCmds=LogPython Log" -stdout
```

Require both native `GUNNER_LOOK_COMPLETE failures=0` summaries and `GUNNER_LOOK_PIE_COMPLETE cycles=2`; cycle completion alone does not certify passing checks. The driver temporarily disables background CPU throttling in memory, restores it on exit, and limits this session to 60 FPS. Keep physical mouse input out of the test viewport. `PlayerController::InputKey` injects input after the platform layer; these results do **not** exercise AppKit/GCMouse hardware delivery, focus loss or device reconnect. Manual acceptance requires free look before/during/after repeated RMB holds with the actual mouse, including after stopping and restarting Play.

No project compile, Blueprint, Python, asset-load or gameplay errors appeared in the accepted run. Engine/environment warnings: editor layout-version reconciliation, `r.MotionVectorSimulation` render-thread access and one outstanding analytics HTTP request at shutdown. The Game build repeats the installed `MetalShaderConverter/include/metal_irconverter_ext` missing-directory warning. Audio was disabled; cooked builds, Windows and physical controllers were not tested. Prior motion acceptance below is retained; that full probe was not repeated for this platform-input setting.

---

# Movement and weapon sandbox validation — 2026-09-19

Environment: UE 5.8.2, macOS 26.6.2 Apple Silicon, 16 GB physical memory, Xcode 26.6 / Mac SDK 26.5, Metal renderer. This accepts the bounded single-player movement range described below, not the full M1/M2/M3 gates or a finished cover shooter.

| Gate | Actual result / evidence |
|---|---|
| Editor and Game C++ targets | Both succeeded. No project compiler diagnostics. `evidence/motion/build-editor.txt`, `build-game.txt`. Game build is not a cooked package test. |
| Retargeting | Seven source clips retargeted to Manny with finite sampled transforms, matching durations and stable roots. `evidence/motion/retarget-report.json`; actual crouch, sprint, jab and roll poses inspected in game. |
| Authored content | Motion composition and final montage repair commandlets completed with 0 errors / 0 warnings. All eight action montages now use evaluated UpperBody/FullBody slots; repair report retained. |
| Live play | Two complete rendered PIE sessions, 92 checks each: **184 passes, zero failures**, two native summaries and driver completion. `evidence/motion/editor-smoke.txt`. |
| Armed motion / camera | WASD travel and live AnimInstance speed, sprint, stopping, shoulder ADS/FOV, Q shoulder movement, real 62 cm crouch and clearance rejection. Crouched ADS remains stationary and can fire. |
| Rifle / pistol | Automatic rifle repeats; held pistol fires once; successive presses fire again; actual target hits; reload conserves ammunition and commits only after completion; interruptions/sprint leave ammo unchanged. |
| Evaluated action animation | Reload slot weight reaches 1 and left hand moves over 36 cm from its preceding pose; FullBody melee slot weight reaches 1. Rendered reload/melee/roll captures show the action poses. |
| Cover | 135 cm reach rejection, 52 cm low-cover attachment, crouch, wall shuffle/end stop, pop-up ADS, high-cover classification and protected fire rejection; selected high edge physically steps out, damages the target beyond the wall, and returns to its anchor on ADS release. |
| Obstruction / melee | Body-to-muzzle and muzzle-to-target obstruction prevent wall penetration. A jab commits one nearby target hit; intervening walls block it. Standing melee locks travel/crouch; crouched, low-cover and simultaneous stance/action conflicts are guarded. |
| Dodge | E plays the acquired roll while a native movement source advances through a clear route; bounded travel, wall rejection, crouch rejection, completion stop, interruption cleanup and no residual slide. |
| Possession lifecycle | Held combat clears on unpossess, only the owned input context is removed, and repossession restores movement without stale firing. |
| Visual inspection | Untouched engine screenshots inspected: crouch/ADS, sprint, rifle/pistol, reload, jab, roll, low/high cover and exposed fire. Selected captures below. |

## Reproduce the motion checks

Run one Unreal process at a time:

```bash
./Tools/build_macos.sh GunnerEditor
./Tools/build_macos.sh Gunner
./Tools/open_editor_macos.sh -GunnerMotionSmoke -nosound \
  "-ExecCmds=py $(pwd)/Tools/validate_motion_editor.py" "-LogCmds=LogPython Log" -stdout
```

The driver runs two real Play in Editor worlds, then exits. Both `GUNNER_MOTION_COMPLETE failures=0` summaries are required; a driver completion marker alone is insufficient. It temporarily disables editor background CPU throttling in memory and restores it before exit; no persistent preference is saved. Frame rate is capped at 60 for this session. Avoid mouse input into the viewport during the synthetic input checks. For a standalone editor-hosted game window, use `./Tools/run_macos.sh -GunnerMotionSmoke -stdout`; close it after completion. The native log is under `~/Library/Logs/Unreal Engine/GunnerEditor/` and raw run logs/screenshots stay in ignored `Saved`.

## Selected visual evidence

- [Animated crouch](evidence/motion/motion_crouch.png) and [stationary crouched ADS](evidence/motion/motion_crouch_ads.png)
- [Sprint](evidence/motion/motion_sprint.png) and [roll](evidence/motion/motion_roll.png)
- [Rifle reload](evidence/motion/motion_reload.png), [pistol fire](evidence/motion/motion_pistol_fire.png) and [melee jab](evidence/motion/motion_melee.png)
- [Low-cover stance](evidence/motion/motion_low_cover.png) and [high-cover exposed firing](evidence/motion/motion_high_cover_fire.png)

## Fixes, warnings and limits

Live inspection caught an initial montage-slot assignment bug: Unreal Python returned a copy of an array element, leaving actions on DefaultSlot. The creation tools now assign the modified struct back into the array, assert the selected slot, and the existing eight montages were repaired with backups. Tests verify evaluated action poses as well as gameplay state. The test driver also uses the native reflected editor setting name for UE 5.8; failed startup attempts are not included as successful runs. Earlier heavily throttled background sessions are diagnostic failures, not acceptance evidence.

Project compile, Blueprint, Python, asset-load and gameplay errors are absent from the accepted runs. The installed engine emits an `r.MotionVectorSimulation` render-thread warning; the Game build reports a missing bundled `MetalShaderConverter/include/metal_irconverter_ext` include directory and reminds that a native executable needs staged content. Engine analytics may report an outstanding HTTP request during shutdown. These are recorded separately from project diagnostics. The retarget commandlet emitted 14 engine warnings about Interchange lazy dependencies/missing curves while producing valid output; final composition and slot repair emitted none. Audio was disabled for gameplay validation and is not certified.

This remains provisional motion, not a polished animation contract: no dedicated wall-entry/exit/lean poses, cover corners, vault, knife mesh/knife-specific attacks or directional crouched ADS locomotion. Protected low-cover head height and torso-layer fade are measured in the live probe. Reload/equip require standing outside low cover; crouching during those actions is rejected. Low-cover protection uses the acquired crouch gait; high-cover exposure uses the acquired armed directional gait. Melee uses a standing punch clip with the currently equipped weapon retained. Roll uses an in-place animation plus guarded native displacement, retains the standing collision capsule and grants no invulnerability. Contact/weapon grip, foot sliding, transitions and camera composition still need production polish. No physical-controller hardware, cooked package, Windows, local duo, LAN or representative encounter performance benchmark was tested. No memory-exhaustion event occurred in the accepted run; this is not a memory budget or leak certification.

---

# Historical M0 foundation validation — 2026-09-19

M0 is validated on UE 5.8.2, macOS 26.6.2 Apple Silicon, Xcode 26.6 / Mac SDK 26.5, Metal rendering. This is foundation evidence, not acceptance of the horde game or the M1 animation pipeline.

| Gate | Evidence | Result |
|---|---|---|
| Editor C++ build | `Tools/build_macos.sh`; `evidence/build-editor.txt` | Succeeded, no project compiler errors/warnings |
| Game C++ build | `Tools/build_macos.sh Gunner`; `evidence/build-game.txt` | Succeeded, no project compiler errors/warnings; not a cooked/package test |
| Authored assets/map bootstrap | Editor Python commandlet; `evidence/bootstrap.txt` | Success, 0 errors and 0 warnings |
| Launch/possession | Real rendered standalone run, then two full PIE cycles | Exactly one Gunner pawn, project controller/input context, valid AnimBP instance |
| Keyboard movement | Synthetic W/D key presses through PlayerController → Enhanced Input → CharacterMovement | Forward/strafe movement, release stopping, opposite W/S cancellation passed |
| Mouse look | Synthetic MouseX/MouseY events | Yaw rotates, positive MouseY looks up |
| Controller axes | Synthetic Gamepad_RightY and Gamepad_LeftY events | Stick up looks up; left stick moves forward; physical hardware not tested |
| Traversal fallback | Space action on actual pawn | Airborne movement and subsequent landing passed |
| Capsule/world collision | Live capsule dimensions, settling and wall test | Radius 36/half-height 90 cm; floor and boundary block correctly |
| Camera obstruction | Live spring-arm collision flag plus rendered capture | Retracts at rear wall; `foundation_camera.png` shows the closer camera |
| Possession lifecycle | Unpossess/context removal/repossess/strafe in each cycle | Owned context removed and controls restored |
| Play lifecycle | Two PIE sessions, each with 20 live checks | 40 passes, two `GUNNER_SMOKE_COMPLETE failures=0`; both worlds torn down; editor exits cleanly |
| Visual inspection | Captures below, inspected after final run | Mannequin idle, moving and airborne poses, scene materials and camera retraction visible |

The final diagnostic extracts are in `evidence/editor-smoke.txt`. They contain measured live checks, not assertions derived from file contents. They also record both cycle completion events and world teardown. The editor closed cleanly after the second teardown before the optional final Python `GUNNER_EDITOR_CYCLES_COMPLETE` marker appeared; both native smoke summaries and both cycle-finished markers are present.

## Reproduction

Build both targets as in README. Then:

```bash
./Tools/run_macos.sh /Game/Gunner/Maps/L_Foundation -GunnerSmoke -stdout
./Tools/open_editor_macos.sh /Game/Gunner/Maps/L_Foundation -GunnerSmoke "-ExecCmds=py $(pwd)/Tools/validate_editor.py" -stdout
```

Run one mode at a time. The full editor driver exercises two sessions. Both native summaries must report zero failures; the driver completing alone is not proof of success. Native logs on this Mac are under `~/Library/Logs/Unreal Engine/GunnerEditor/`. The standalone helper stays open; the editor driver requests exit when finished. Avoid moving the mouse in the captured viewport during synthetic input checks.

## Visual evidence

Final PIE captures are 2027 x 1090 pixels. They are untouched engine screenshots.

- [Idle/shoulder view](evidence/foundation_idle.png)
- [Moving mannequin](evidence/foundation_move.png)
- [Airborne pose](evidence/foundation_jump.png)
- [Camera retraction near wall](evidence/foundation_camera.png)
- [Movement after repossession](evidence/foundation_repossess.png)

These prove the baseline renders and changes pose in the launched scene. They do not certify foot sliding, full transition quality, crouch, armed layers or retargeting. Those require M1 motion review with a complete animation set.

## Fixes made during validation

- Corrected the GameMode translation unit to include its own header first, resolving Unreal build-tool include-order diagnostics.
- Updated bootstrap Python to the installed 5.8 struct APIs and removed an editor-viewport call from commandlet generation. Rebuilt authored assets successfully; partial generated output was preserved outside the repository.
- Made smoke initial camera orientation independent of desktop cursor motion during launch and bounded gameplay pitch to -60°/+50°.
- Disabled legacy controller input scaling and removed redundant pitch inversion in Enhanced Input, then verified upward mouse and stick input in the live pawn.
- Delayed the smoke harness teleport until after the camera-obstruction screenshot so the capture shows the tested state.

## Warnings and known limits

No project compile, Blueprint, Python, asset-load or gameplay errors were present in the final validation. A final Game-target build also reports a bundled-engine include-directory warning for `MetalShaderConverter/include/metal_irconverter_ext`; the target still builds successfully. This is an installed engine SDK-layout issue, not a project source diagnostic. The final editor log still reports an AudioUnit sample-rate query warning (`2003332927`) and an engine `r.MotionVectorSimulation` render-thread safety warning. Audio is outside M0 and was not certified. Earlier standalone editor-hosted runs also emitted Unreal editor data-storage widget-registration warnings. Do not describe this environment as universally warning-free.

No physical controller, cooked package, Windows build, performance benchmark, second local player or LAN validation was performed. Editor background throttling and first-load shader compilation make these runs unsuitable for performance claims. At extreme wall proximity the character occupies much of the frame; camera fade/shoulder switching and cover-camera polish remain later work.

At the M0 release, crouch, sprint, aim, weapons and cover were disabled. The movement sandbox above supersedes those feature limits. The preserved foundation map still uses its original unarmed composition; no full Warden animation contract, horde loop, vault, revival or pulse-defense acceptance is implied.
