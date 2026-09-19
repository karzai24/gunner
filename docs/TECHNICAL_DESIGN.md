# Unreal technical design

## Baseline and first milestone

Engine inspected: Epic Launcher UE 5.8.2 (CL 56702186), Apple Silicon macOS 26.6.2, Xcode 26.6, Mac SDK 26.5, Metal toolchain installed. Project: `Gunner.uproject`; one runtime module, `Gunner`, and Game/Editor targets. AndroidFileServer is disabled because Android deployment is outside this foundation; its editor-generated local connection settings are not source-controlled. No existing Unreal project or Godot source was present. The remote's only commit was a README. Historical design documents are preserved under `reference/godot/` and do not certify the new implementation.

M0 establishes a saved 44 x 32 m test map, a composed mannequin character, collision-aware shoulder camera, locomotion/jump, and Enhanced Input. It deliberately has no combat, crouch action, sprint action, cover logic, enemies, menu, network session or match loop. Metric cover blocks are collision/camera fixtures, not working cover. The map is a metric test room, not the complete Rift Bastion layout.

## Implemented structure

| Location/type | Responsibility |
|---|---|
| `Source/Gunner` | Runtime C++ module, no UnrealEd dependency |
| `AGunnerCharacter` | ACharacter/CharacterMovement, capsule, spring arm, camera, input handlers; supports Blueprint composition |
| `AGunnerPlayerController` | Adds/removes its own context in the owning LocalPlayer subsystem on possession, acknowledgment, setup and teardown |
| `UGunnerInputConfig` | Designer-owned context and action references; avoids runtime key construction |
| `AGunnerGameMode` | Default controller/pawn types; Blueprint selects the composed pawn |
| `Content/Gunner/Characters/BP_Warden` | Manny mesh and Animation Blueprint; input configuration |
| `Content/Gunner/Input` | Move, mouse/stick look and traversal actions; mapping context and data asset |
| `Content/Gunner/Core/BP_GunnerGameMode` | Foundation composition |
| `Content/Gunner/Maps/L_Foundation` | Saved, editable map with spawn, collision and metric fixtures |
| `Content/Characters/Mannequins` | Epic template skeleton, animations, rig, mesh and materials |
| `Tools` | Reproducible editor bootstrap and local build/run entry points |

Keyboard axes combine cumulatively so opposing directions cancel; CharacterMovement constrains diagonal speed. Mouse input is a delta; stick input is a rate scaled by delta time, with a dead zone. Legacy controller input scaling is explicitly disabled, so it cannot double-apply sensitivity or pitch inversion. Positive MouseY/right-stick Y means look up; camera pitch is limited to -60°/+50°. The traversal action is currently only the grounded jump fallback. Gamepad mappings are authored for one local player; device assignment for a second player is M4.

No global input clearing or actor discovery happens during normal gameplay. Do not create gameplay singletons to replace Unreal ownership. The controller's possession hooks are future-compatible seams, not evidence that multiplayer has been implemented.

## Planned ownership and replication boundaries

| System | C++/Unreal owner | Blueprint/data authoring | Authority |
|---|---|---|---|
| Match and waves | GunnerGameMode + wave director component; timers and reserved/live counts | Wave Data Assets | Server only |
| Shared match presentation | GunnerGameState; phase, wave number, remaining enemies, outcome | Per-player UMG views | Server writes; clients observe |
| Player identity | GunnerPlayerState with stable content ID | Selection UI | Server validated |
| Health/incapacitation/revival | Replicated Health component and guarded life state | Reaction/revive presentation | Server owns damage, timers and revive checks |
| Weapon/ammunition | Weapon actor/component; camera target then muzzle obstruction trace | Weapon Data Asset, montages, Niagara | Server validates rate, aim, ammo and reload; local cosmetics |
| Locomotion | CharacterMovement; custom movement mode only when cover/vault requires it | Animation Blueprint/blend spaces/aim offsets | Native movement prediction; extend saved moves when adding predicted state |
| Cover/vault | Character component with bounded geometry queries and explicit attachment state | Cover metadata, montages, Motion Warping | Server-validated geometry and destination |
| Enemy | AIController, perception, navigation; start with Behavior Tree | Blackboard/BT, enemy Data Asset | Server |
| Environmental pulse | Replicated defense actor/component | Field shape, VFX, cooldown display | Server activation, slow and damage; 8-second baseline |
| Local session/travel | GameInstance subsystem, LocalPlayers; ordinary level travel | Frontend widgets | Local selection now; OnlineSubsystem sessions later |
| UI | Owning-player UMG; delegates/view models tied to appropriate state owners | Widget Blueprints | Passive presentation, no gameplay decisions |
| Audio/settings | Audio Mixer/submix/concurrency; GameUserSettings/SaveGame | Sound assets, settings UI | Local preferences; versioned validation |

LAN is later. Do not implement custom ENet snapshots, rollback or lag compensation in M0. Choose listen-server sessions and native replication first when LAN work is authorized. No GAS dependency until ability complexity justifies it. StateTree/EQS are optional based on actual AI needs.

## Units and tuning

Unreal is centimeters, Z up, X forward. Capsule sizes use radius and half-height: 36/90 cm standing, 36/62 crouched. The crouch value is reserved but disabled pending animation. Camera arm 320 cm with 50 cm shoulder offset and 65 cm target height offset. Spring-arm sweep handles occlusion. Cover dimensions and 44 x 32 m floor match source metrics without intentional dimension deviations.

M0 provisional locomotion tuning: 450 cm/s maximum, 1200 cm/s² acceleration, 1600 cm/s² braking, 480°/s yaw rotation, jump velocity 440 cm/s, 75° field of view. These values are new tuning choices, not recovered prototype values. Foot planting and heavy movement feel remain M1 acceptance work.

Use a conventional raster baseline (no Lumen, virtual shadow maps or motion blur) to establish correctness on this 16 GB development Mac. This is not an eventual performance promise. Profile cooked builds and two local views before adopting expensive rendering features.

## Official references

- [Enhanced Input](https://dev.epicgames.com/documentation/unreal-engine/enhanced-input-in-unreal-engine)
- [C++ character movement](https://dev.epicgames.com/documentation/unreal-engine/coder-03-configure-character-movement-with-cplusplus-in-unreal-engine)
- [macOS requirements](https://dev.epicgames.com/documentation/en-us/unreal-engine/macos-development-requirements-for-unreal-engine)

Installed 5.8 headers and templates govern API details. See `TEST_MATRIX.md` for measured validation rather than inferring it from these references.
