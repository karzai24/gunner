# Project Gunner: Rift Bastion

An original cooperative third-person shooter being rebuilt in **Unreal Engine 5.8.2**, using C++ gameplay and Blueprint content.

**Current scope: movement and weapon sandbox.** The new range composes a shoulder camera, armed movement, crouch, sprint, a dodge roll, rifle/pistol aiming and shooting, reload/equip animations, a melee jab, and high/low cover attachment with a physical step out at valid high-cover edges. The sandbox has passed build and live gameplay checks; see [TEST_MATRIX](docs/TEST_MATRIX.md) for actual validation and limitations. This is not yet the three-wave horde game or a complete polished cover-shooter movement set. Character art remains an Epic mannequin placeholder.

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
| Reload while standing | R | Left face button |
| Rifle / pistol while standing | 1 / 2 | D-pad up / down |
| Melee jab | F | Right-stick click |
| Toggle crouch | C or Left Ctrl | Right face button |
| Sprint | Hold Left Shift while moving forward | Hold left-stick click |
| Dodge roll | E | Left shoulder button |
| Attach to cover / detach / jump fallback | Space | Bottom face button |
| Jump without seeking cover | J | No separate mapping |
| Swap camera shoulder | Q | Right shoulder button |

Escape stops Play in Editor. Close the standalone window to exit. Controller mappings are authored; physical hardware verification is recorded separately in the test matrix.

ADS tightens the over-the-shoulder camera and aiming pose. The rifle fires automatically while held; the pistol fires once per press. Range targets show damage and reset after depletion. The HUD shows ammunition, current action, hit feedback and blocked muzzle feedback. There is no enemy AI or player health loop.

## Current movement limits

- **Crouch uses real imported motion.** The free source provides idle and forward movement, so crouched travel turns the body toward movement. Outside ADS, the armed upper-body layer fades out to preserve the source's low protective torso pose. Crouched ADS restores the armed pose and is stationary until proper directional crouch clips are available.
- **Reload and weapon switching require standing.** They are blocked while crouched, requesting crouch, or attached to low cover, including its standing ADS position, because only upright handling clips are available. Detach from low cover and stand first. Crouch input is blocked during reload/equip; standing high-cover reload remains available.
- **Cover supports attachment and movement along a static wall.** Low cover crouches while protected and requests standing when aiming. At a valid high-cover edge, holding ADS physically steps the standing character out; releasing ADS returns to cover. Q chooses the shoulder. High-cover firing requires ADS at that open edge, and shots still check obstruction between body, muzzle and aim target.
- **Roll uses a real retargeted animation** and travels up to 350 cm along movement, or camera facing when stationary. It requires standing, grounded clearance and supported floor; crouch, cover, reload, equip and melee block it.
- Dedicated wall-lean poses, corner turns, cover entry/exit montages, vaults and knife handling are not implemented. High-cover peeking uses the existing directional armed gait; melee uses a standing jab.
- Sprint and roll lower combat readiness; airborne combat and crouched melee are disabled. There is no local duo, LAN, waves, revival, pulse defense, menu or packaged release in this sandbox.

All motion comes from installed Epic template clips and licensed **Quaternius CC0 animations retargeted to Manny**. No custom character art or manually keyed replacement motions were created. [Asset research](docs/ASSET_RESEARCH.md) records free coverage and optional paid sources; no pack purchase was made.

## Development and validation

Read [AGENTS.md](AGENTS.md), [technical design](docs/TECHNICAL_DESIGN.md), [milestones](docs/MILESTONES.md), [animation contract](docs/ANIMATION_PIPELINE.md) and [asset provenance](docs/ASSET_PROVENANCE.md).

`Source/Gunner` owns runtime behavior. `Source/GunnerEditor` contains editor-only asset authoring helpers. Project assets live under `Content/Gunner`; installed Epic package paths remain under `Content/Characters` and `Content/Weapons`. Editable animation source and licenses are retained under `ArtSource`. Binary assets are stored directly for this prototype; adopt Git LFS before substantially expanding production art history.

The maps and assets are already authored. **Do not rerun `create_foundation.py`, `import_motion_source.py`, `retarget_motion.py` or `create_motion_sandbox.py` for ordinary setup.** The one-time `install_dodge.py` and `repair_motion_montage_slots.py` migrations are already applied. Their creation guards preserve existing assets; they are reproduction/authoring tools, not launch requirements.

The opt-in live range probe is:

```bash
./Tools/run_macos.sh -GunnerMotionSmoke -stdout
```

It exercises live input and gameplay, logs `GUNNER_MOTION_COMPLETE failures=0` only when its checks pass, and captures screenshots under `Saved/Screenshots`. Its existence is not a test result; consult the test matrix and inspect actual motion. The old `-GunnerSmoke` probe belongs to the preserved foundation scenario.

To compile the Game target, use `./Tools/build_macos.sh Gunner`; its executable needs cooked content. The run script uses the editor's uncooked game mode. Windows setup requires UE 5.8 and its supported Visual Studio C++ toolchain; Windows and a cooked release have not been validated for this pass.

Original supplied documents remain in `docs/reference/godot`. The former remote contained only a README and was backed up before replacement; no inspected Godot implementation or inherited Godot test results are claimed.
