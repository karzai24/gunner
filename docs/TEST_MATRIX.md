# Unreal foundation validation — 2026-09-19

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
./Tools/run_macos.sh -GunnerSmoke -stdout
./Tools/open_editor_macos.sh -GunnerSmoke "-ExecCmds=py $(pwd)/Tools/validate_editor.py" -stdout
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

Crouch, sprint, aim, weapons, enemies, waves, results, cover/vault, revive and pulse defenses are not implemented. Crouch is intentionally disabled rather than showing an incorrect standing pose. Manny is temporary; the skeleton/retargeting strategy is defined, but no second-source retarget or complete Warden animation contract has passed yet.
