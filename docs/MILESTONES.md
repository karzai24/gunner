# Milestone acceptance gates

Every milestone requires a successful build, a real launch, exercised gameplay, visual inspection, fixed attributable errors/warnings, evidence and limitations in TEST_MATRIX. A checkbox/compilation/import is not evidence by itself.

## M0 — Foundation (current scope)

- Editor and game target compile on inspected UE version.
- Project opens a saved test map and Play possesses exactly one BP_Warden through the project GameMode/PlayerController.
- Authored Enhanced Input actions/context/config drive camera-relative movement, mouse/stick look and grounded jump.
- Capsule is radius 36/half-height 90 cm; floor/walls collide; shoulder spring arm retracts near geometry.
- Manny-compatible mesh visibly animates idle and moving, with working airborne transition.
- Repeat play/possession has no duplicate mappings; game ends/starts cleanly.
- Unreal AGENTS, architecture, migration, provenance, animation plan and milestone criteria exist.
- No crouch, sprint, weapon, enemy or cover claims; no full horde game in this pass.

## M1 — Animation/locomotion proof (blocks combat expansion)

- Canonical skeleton and rest pose documented; a second compatible humanoid or animation source passes IK retargeting, scale/root/hand/foot checks.
- Real standing walk/run, crouched idle and directional locomotion, aim movement and sprint animate coherently during transitions.
- Clearance-safe crouch/stand; blocked standing preserves crouch. Camera respects geometry during all stances. No capsule-only fake crouch.
- Weapon sockets, grip alignment, aim offsets/layered blending and root-motion policy proven in an animation test map.
- Catalog and preview cover enter/idle/move/exit, vault, fire, reload, hit, incapacity and death assets, with provenance and explicit gaps; no large gameplay implementation while required animation coverage is missing.
- Capture representative views and retarget comparisons, including foot sliding and shoulder deformation observations.

## M2 — First solo playable slice

- One player, walk/run/sprint/proper crouch/aim/fire/reload/health, one Veyra archetype, spawning, exactly three escalating waves.
- Authored Rift Bastion greybox with 44 x 32 m footprint, lanes >=4.5 m, reactor landmark and readable breach routes.
- Reservation-through-death accounting never understates remaining threat. Damage/ammo/reload/spawn/outcome are authority-owned.
- Final enemy death produces victory; terminal solo death produces defeat; restart and menu paths work repeatedly.
- Incapacitation timing must preserve the design's solo expiry rule before declaring full rule parity.
- Shots cannot pass through muzzle-obstructing cover even if the reticle sees a target.
- Profile a representative wave and document frame times/memory; this gate does not claim split-screen or LAN.

## M3 — Cover and traversal

- Bounded cover detection at 135 cm, attach at 52 cm, lateral movement and safe detach; high/low classification matches 180/115 cm fixtures.
- Standing/crouched cover locomotion, valid aiming/firing and guarded stance transitions; state and camera remain coherent.
- Context traversal prioritizes cover/exit/vault then grounded jump; blocked vault destination rejects cleanly; montage interruption cannot strand state.
- Test corners, low ceilings, flanks, muzzle obstruction and collision during vault; inspect real poses at gameplay camera distance.

## M4 — Local cooperative slice

- Two distinct LocalPlayers, pawns, cameras and HUDs; keyboard/mouse P1 plus controller P2 with no input bleed.
- Incapacitation, valid teammate revive, expiry/death, all-configured-players-dead defeat and restart verified.
- Two pulse defenses: 8-second active slow/damage, cooldown rejection and valid AI bypass routes.
- Actual controller hardware tested; readable split-screen at target resolutions; profile heaviest two-view encounter.

## M5 — LAN

- Host/join, authoritative damage/ammo/waves/revive/defenses, replicated presentation and native movement prediction verified on separate processes/machines.
- Late join/disconnect, invalid requests, latency/loss and session teardown tested; document supported player topology including local duo interactions.
- Never label a declared boundary or a successful connection as gameplay networking completion.
