# Technical Design

## Runtime ownership

`Bootstrap` owns top-level presentation. `GameFlow` owns application state and requests scene travel through `LevelManager`. `SessionManager` owns local player count/loadouts and the future ENet transport boundary. `HordeMatch` owns authoritative match rules, player/enemy spawning, pooling, HUD view models, and results.

Actors compose `HealthComponent`, `WeaponComponent`, input snapshots, and explicit state machines. Components signal upward; the match coordinates siblings. No HUD polls the scene tree: players emit snapshots to their paired HUDs and the match pushes shared wave state.

## States

- Game: Boot → FrontEnd → Loading → InGame → Paused/Results → FrontEnd, with Failure recovery.
- Player locomotion: Grounded, Crouched, Sprint, Airborne, Cover, Vault, Incapacitated, Dead.
- Player action: Free, Aiming, Reloading, Disabled.
- Weapon: Ready, Firing, Reloading, Empty, Disabled.
- Enemy: Inactive, Spawn, Pursue, Attack, Stagger, Dead.
- Match: Warmup, Intermission, Active, Victory, Defeat.

All transitions are committed by the owning machine using enumerated state IDs and guarded transition tables.

## Simulation and authority

Input is captured into reusable per-player snapshots and consumed at 60 Hz. The local match is server-equivalent authority: it owns spawns, damage, health, ammo, deaths, wave counts, and results. Presentation recoil, procedural limb motion, audio, and VFX do not affect shot direction.

Crouch changes a pawn-local duplicated capsule rather than the shared scene resource, and standing is guarded by a bounded physics query over only the added headroom. Cover owns an explicit upright/crouched sub-state so stance changes do not detach the pawn; this sub-state drives collision height, camera height, lateral speed, HUD state, exit stance, and vault guards. Cover and jump share a contextual traversal input: cover consumes the command when a valid transition exists; otherwise the locomotion state machine may commit a grounded jump.

`SessionManager` exposes offline/local/host/client modes and authority checks. Future LAN clients will send bounded input snapshots with sequence/tick; the authoritative host will replicate compact actor state. Current LAN transport methods are not exposed in the menu because gameplay replication and reconciliation are not yet implemented.

## Performance

The match preallocates the maximum Veyra population and a fixed tracer/impact pool before combat. Combat ticks do not instantiate scenes, load resources, grow unbounded collections, create tweens, or search groups. AI navigation target refresh is staggered and rate-limited.

## Persistence

Settings use a versioned JSON schema written via temporary file and rename. Invalid or corrupt files fall back to validated defaults without deleting the source. No trusted match outcome is stored.
