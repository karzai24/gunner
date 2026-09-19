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
