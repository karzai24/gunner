# Project Gunner: Rift Bastion

An original cooperative third-person shooter being rebuilt in **Unreal Engine 5.8.2**, using C++ gameplay and Blueprint content.

**Current scope: movement and weapon sandbox.** The range includes a shoulder camera, armed movement, crouch, sprint, dodge roll, rifle/pistol aiming and shooting, protective reload/equip, standing jab/cross melee, jump transitions, empty-trigger feedback and bounded cover combat. The local movement profile adds smooth cover approaches, contextual tap/hold traversal, a hunched rifle sprint, directional crouched ADS and rifle wall-cover poses. The committed Blueprint remains portable; the additional licensed animation files stay local. See [TEST_MATRIX](docs/TEST_MATRIX.md) for actual checks and limitations. The complete movement system and three-wave horde game remain unfinished; character art is still an Epic mannequin placeholder.

## Open and play on macOS

Requires UE 5.8 with C++ support, Xcode and its Metal toolchain. The development machine has UE at `/Users/Shared/Epic Games/UE_5.8`; set `UE_ROOT` if yours differs.

```bash
./Tools/build_macos.sh
./Tools/open_editor_macos.sh
```

You can also double-click `Gunner.uproject`. The default map is **`Content/Gunner/Motion/Maps/L_MotionRange`**. Press **Play** to enter the range. If an older editor session restores the foundation map, open `L_MotionRange` from the Content Browser. The original `Content/Gunner/Maps/L_Foundation` is preserved.

For a separate editor-hosted game window:

```bash
./Tools/run_macos.sh
```

| Action | Keyboard / mouse | Controller mapping |
|---|---|---|
| Move / look | WASD / mouse | Left / right stick |
| Shoulder ADS / fire | Hold RMB / LMB | Left / right trigger |
| Blind fire over low cover | LMB while attached, without RMB | Right trigger without left trigger |
| Reload standing or crouched, including cover | R | Left face button |
| Rifle / pistol standing or crouched | 1 / 2 | D-pad up / down |
| Melee jab / cross (alternating presses) | F | Right-stick click |
| Toggle crouch | C or Left Ctrl | Right face button |
| Sprint | Hold Left Shift while moving forward | Hold left-stick click |
| Dodge roll | E | Left shoulder button |
| Context traversal with local profile | Tap Space for cover or open-space roll; hold for sprint / fast wall travel | Bottom face button |
| Jump without seeking cover | J | No separate mapping |
| Swap camera shoulder | Q | Right shoulder button |

With the optional local profile enabled, Space seeks nearby cover on press, rolls on a short open-space tap, and runs after a 0.18-second hold. While attached, hold it with wall-tangent input for faster travel; moving away releases cover. A second press cancels an unfinished approach. The portable baseline keeps Space cover/detach/jump behavior. J always requests a separate jump. [Local movement setup](docs/LOCAL_MOVEMENT_SETUP.md) explains the profile and fallback.

Escape stops Play in Editor. Close the standalone window to exit. Controller mappings are authored; physical hardware verification is recorded separately in the test matrix.

The Mac build uses Unreal's AppKit mouse-input path as a compatibility workaround for mouse look freezing after ADS. Restart Unreal after updating this setting; it is read only at startup. Mouse look should remain active while aiming and after releasing RMB. Physical-mouse confirmation of this workaround is still pending; see the latest test matrix entry.

ADS tightens the over-the-shoulder camera and aiming pose. The rifle fires automatically while held; the pistol fires once per press. Range targets show damage and reset after depletion. The HUD shows ammunition, current action, hit feedback and blocked muzzle feedback. An empty trigger plays a brief handling animation and displays `EMPTY / R RELOAD`; reload can interrupt it immediately. There is no enemy AI or player health loop.

## Current movement and limits

- **Crouch uses real motion.** The local profile has genuine forward/backward/left/right crouch clips for free movement, crouched ADS and high-cover edge steps. Stationary free/high-cover crouch entry/exit and arm-only weapon carry passed the focused rendered checks, keeping the authored torso and legs. Low cover retains its separately validated protective crouch; the taller directional gait is not used there. The portable baseline keeps forward crouch and stationary crouched ADS.
- **Reload works standing or crouched, including low cover.** R layers the existing rifle/pistol reload motion onto the arms while preserving the real crouch torso and legs. Reloading during low-cover ADS lowers the character until the reload finishes; held ADS then resumes. Stance toggles are disabled during reload/equip. Weapon switching also preserves crouch through an arm-only handling layer; switching during low-cover ADS lowers the character until the action finishes.
- **Cover entry now moves through a checked approach.** It rejects blocked or unsupported routes and cancels safely if geometry changes. The local profile adds actual standing rifle wall idle/left/right poses, a corrected support-hand grip and faster wall travel. Low-cover LMB raises the gun for guarded blind fire while the head stays down; RMB stands for aimed fire. At a valid high-cover edge, ADS physically steps out and release returns; directional crouch coverage also permits a crouched edge step. Q selects the shoulder, and firing still checks body/muzzle/target obstruction.
- **Roll uses a real retargeted animation** and travels up to 350 cm along the chosen direction. It can start from crouch or cover only when the full standing capsule and supported route are clear. Rolling into the wall rejects without losing cover. Reload, equip and melee block it; held ADS resumes after the roll.
- **The local rifle sprint uses a hunched run at 500 cm/s**, with 300 cm/s normal travel, a lower camera and a 160-degree/s turn limit independent of free mouse look. Pistol sprint retains its existing source. A genuine braced knee slide, corner turns, cover transfers, authored entry/exit actions, vaults, pistol wall poses and knife handling remain disabled or missing. Acquired transition candidates need contact, route and interruption validation before use.
- Sprint and roll lower combat readiness; airborne combat and crouched melee are disabled. There is no local duo, LAN, waves, revival, pulse defense, menu or packaged release in this sandbox.

The portable composition combines installed Epic template clips and **Quaternius CC0 animations retargeted to Manny**. The optional local profile additionally uses compatible installed Mover animation copies and officially downloaded Mixamo animations. Those sources and derivatives are ignored by Git; cloning the repository does not acquire them. Low-cover blind fire uses native arm IK and the existing fire clips over the genuine crouch animation; no replacement motion tracks are manually keyed. No custom character art was created. Jump takeoff/landing, dry fire and the second melee attack now reuse those existing clips. [Animation backlog](docs/ANIMATION_BACKLOG.md) distinguishes integrated actions from remaining candidates. [Asset research](docs/ASSET_RESEARCH.md) records free coverage and optional paid sources; no pack purchase was made.

## Development and validation

Read [AGENTS.md](AGENTS.md), [technical design](docs/TECHNICAL_DESIGN.md), [milestones](docs/MILESTONES.md), [animation contract](docs/ANIMATION_PIPELINE.md) and [asset provenance](docs/ASSET_PROVENANCE.md).

`Source/Gunner` owns runtime behavior. `Source/GunnerEditor` contains editor-only asset authoring helpers. Project assets live under `Content/Gunner`; installed Epic package paths remain under `Content/Characters` and `Content/Weapons`. Editable animation source and licenses are retained under `ArtSource`. Binary assets are stored directly for this prototype; adopt Git LFS before substantially expanding production art history.

The maps and assets are already authored. **Do not rerun `create_foundation.py`, `import_motion_source.py`, `retarget_motion.py` or `create_motion_sandbox.py` for ordinary setup.** The one-time `install_dodge.py`, `install_blind_fire.py`, `install_crouch_reload.py`, `install_motion_polish.py` and `repair_motion_montage_slots.py` migrations are already applied. Their creation guards preserve existing assets; they are reproduction/authoring tools, not launch requirements.

The recovered movement profile and portable fallback passed **1,516 live checks across twelve rendered PIE sessions, zero failures**. The traversal suite covers 30 scenarios per session; `-GunnerRequireTraversalCapabilities` rejects missing local capabilities. Fresh post-editor persistence also passed 2,013 sampled poses across eleven Mixamo clips and fourteen package hashes. [TEST_MATRIX](docs/TEST_MATRIX.md) records the repaired auto-reimport issue, actual evidence and remaining limits.

The existing opt-in live range probe is:

```bash
./Tools/run_macos.sh -GunnerMotionSmoke -stdout
```

It exercises live input and gameplay, logs `GUNNER_MOTION_COMPLETE failures=0` only when its checks pass, and captures screenshots under `Saved/Screenshots`. Its existence is not a test result; consult the test matrix and inspect actual motion. The old `-GunnerSmoke` probe belongs to the preserved foundation scenario.

To compile the Game target, use `./Tools/build_macos.sh Gunner`; its executable needs cooked content. The run script uses the editor's uncooked game mode. Windows setup requires UE 5.8 and its supported Visual Studio C++ toolchain; Windows and a cooked release have not been validated for this pass.

Original supplied documents remain in `docs/reference/godot`. The former remote contained only a README and was backed up before replacement; no inspected Godot implementation or inherited Godot test results are claimed.
