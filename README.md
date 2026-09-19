# Project Gunner: Rift Bastion

An original cooperative third-person horde shooter being rebuilt in **Unreal Engine 5.8.2**, using C++ foundations and Blueprint content.

**Current scope: M0 foundation.** This repository provides a mannequin player, shoulder camera, Enhanced Input and an editable metric test map. The three-wave horde game, proper crouch, sprint, aiming/combat, cover, revive, split-screen setup and LAN are later milestones. Historical Godot feature lists are not claims about this build.

## Open and play on macOS

Requires UE 5.8 with C++ support, Xcode and its Metal toolchain. The development machine has UE at `/Users/Shared/Epic Games/UE_5.8`; set `UE_ROOT` if yours differs.

```bash
./Tools/build_macos.sh
./Tools/open_editor_macos.sh
```

The editor opens `Content/Gunner/Maps/L_Foundation`. Press **Play**. For a standalone editor-hosted game window:

```bash
./Tools/run_macos.sh
```

Controls: **WASD** move, **mouse** look, **Space** grounded jump. Gamepad: left stick move, right stick look, bottom face button jump. Crouch and sprint are intentionally unavailable until their proper animations are integrated. Escape stops Play in Editor; close the window to exit standalone. There is no pause/menu UI yet.

To build the Game target: `./Tools/build_macos.sh Gunner`. Running its executable requires cooked content; `run_macos.sh` uses the editor's uncooked game mode. No packaged release is included.

On Windows, use UE 5.8 with its supported Visual Studio C++ toolchain, generate project files from `Gunner.uproject`, build GunnerEditor Development, and open the project. Windows has not been validated in this milestone.

## Development and validation

Read [AGENTS.md](AGENTS.md), [technical design](docs/TECHNICAL_DESIGN.md), [migration plan](docs/MIGRATION_PLAN.md), [milestones](docs/MILESTONES.md) and [animation contract](docs/ANIMATION_PIPELINE.md).

`Source/Gunner` owns the runtime foundation. `Content/Gunner` contains project Blueprint/data/map assets. `Content/Characters` contains Epic template assets with provenance in [ASSET_PROVENANCE](docs/ASSET_PROVENANCE.md). Binary assets are committed directly for this small foundation; no external LFS fetch is needed. Adopt Git LFS before growing large production art history.

The map and assets are already committed. **Do not run `Tools/create_foundation.py` during ordinary setup.** It is a one-time editor bootstrap, refuses to overwrite existing assets, and is preserved for review/reproduction in a blank project.

Run the opt-in live smoke harness:

```bash
./Tools/run_macos.sh -GunnerSmoke -stdout
```

It drives real key events through Enhanced Input, checks movement/jump/collision/possession, captures gameplay under `Saved/Screenshots`, and logs `GUNNER_SMOKE_COMPLETE failures=0` on success. It does not exit the window. For two full Play-in-Editor cycles (exits that editor when finished):

```bash
./Tools/open_editor_macos.sh -GunnerSmoke "-ExecCmds=py $(pwd)/Tools/validate_editor.py" -stdout
```

The editor driver logs `GUNNER_EDITOR_CYCLES_COMPLETE`; both smoke summaries must also have zero failures. The harness is not spawned in normal gameplay or Shipping. See [TEST_MATRIX](docs/TEST_MATRIX.md) for evidence and limitations; automated checks do not replace animation/visual inspection.

Original supplied documents are preserved in `docs/reference/godot`. The former README-only remote was backed up separately before replacement. No Godot runtime files are part of this Unreal project.
