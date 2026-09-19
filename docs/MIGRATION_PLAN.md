# Migration plan — Godot proof of concept to Unreal

## Evidence and precedence

Read all eleven supplied documents. The prior GitHub repository contained only `README.md` at `7f9c7f94d69b9aea6833406683d408a11380969d`; no behavior could be replayed from that repository. The new implementation uses documented behavior as its reference. Preserve originals in `docs/reference/godot/`; the old AGENTS file is renamed `AGENTS_GODOT_REFERENCE.md` so it cannot apply instructions recursively.

The current user request selects Unreal and supersedes all Godot implementation mandates. Historical release notes/tests describe their previous environment only. Character A history conflicts between older release notes (custom Micah) and the later manifest (Quaternius mannequin); preserve Micah's identity and stable `character_vera` ID, use Epic Manny temporarily, and claim neither historical art asset is present.

## Classify the old AGENTS requirements

| Old requirement | Decision | Unreal implementation |
|---|---|---|
| Original IP, readable cover, camera collision, two-stage aim | Retain design | Art direction, spring arm, future camera-to-target/muzzle obstruction traces |
| Rebindable, isolated player input | Retain design | Enhanced Input contexts per LocalPlayer; remapping UI later |
| Explicit guarded game/player/weapon/AI states | Retain intent | Domain enums/tags, components, AnimBP states and BT where suitable |
| Every active system MUST have a Node FSM | Drop implementation rule | Native engine ownership; avoid ceremonial classes |
| Fixed custom 60 Hz loop and double-buffer render dispatch | Drop | Unreal frame/tick/physics scheduling; do not replace engine timing |
| Zero allocation in every tick, mandatory preallocated pools | Adapt | Avoid needless tick allocation; profile and bound high-frequency spawning |
| SoA/AoSoA above 100 elements | Drop threshold | Profile first; use Unreal AI/actors initially, evaluate Mass only if evidence warrants |
| Call down, signal up; all distant communication via EventBus | Adapt | Typed delegates/interfaces and scoped subsystems; no global player-less events |
| HUD cannot reference a gameplay object | Retain passive UI intent | Owning-player view model/delegates; safe explicit object references allowed |
| Autoload audio/level/camera/save managers | Replace | Audio concurrency/mixer, GameInstance subsystem/travel, PlayerCameraManager, GameUserSettings/SaveGame |
| NodePool3D, Resource, .tres/.tscn and res:// directories | Replace | Actors/components, Data Assets, .uasset/.umap, Source/Content/Config |
| Handwritten input command allocation and custom rollback | Defer/replace | Enhanced Input + CharacterMovement; authoritative LAN and lag work later |
| Godot AnimationTree and procedural limb motion | Replace | Manny skeleton, AnimBP/state machine, blend spaces, IK Retargeter, montages |
| Encryption by default for saves | Drop unsupported mandate | Validate/version preferences; no trusted outcomes persisted |
| Build/run/visual verification and provenance | Retain | Unreal build, launched gameplay evidence, current test matrix and license ledger |

## System migration sequence

| Godot system | Unreal destination | Milestone |
|---|---|---|
| Bootstrap / GameFlow / LevelManager | GameInstance subsystem + GameMode travel/frontend | M0 framework; M2 flow |
| Player.tscn / CharacterBody3D | BP_Warden based on AGunnerCharacter | M0 |
| Input snapshots / CameraManager | Enhanced Input + per-player controller/camera | M0 |
| Hero humanoid / AnimationTree | SK_Mannequin contract, AnimBP, IK rigs/retargeter | M0 baseline; M1 proof |
| Crouch/sprint/aim states | Native CharacterMovement + authored animation layers | M1 |
| Health/weapon/hurtbox | Native actor components and collision channels | M2 |
| HordeMatch / wave reservations | Server GameMode/wave director + GameState | M2 |
| Veyra pool/FSM/navigation | AIController, navmesh, BT; bounded spawner | M2 |
| Rift Bastion procedural map | Authored greybox map; metric test room first | M0 room; M2 arena |
| Cover stance / contextual traversal | Cover component, custom movement when needed, montages/warping | M3 |
| SessionManager local duo | LocalPlayers, viewport split and device routing | M4 |
| Incapacitation/revive/pulse emitters | Authority-owned health/interaction/defense components | M4 |
| ENet boundary | Unreal listen server, replication and session layer | M5 |

## Delivery gates

See `MILESTONES.md`. Establish the full humanoid animation contract before broad gameplay. No final character production before standing/crouched/aiming locomotion, weapon alignment, cover and retargeting are proven. The first playable solo slice follows locomotion validation; cover follows that; local duo/defenses follow stable solo; LAN last.

The old Git mirror is retained outside the new repository at `../Gunner-backups/gunner-godot.git`. Downloaded original documents remain untouched. The old remote was deleted and recreated as `karzai24/gunner` with its previous public visibility. The new repository uses a fresh main branch rather than the old Godot commit history.
