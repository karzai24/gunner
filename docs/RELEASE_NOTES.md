# Xbox-style controller input — 2026-09-20

The movement range now supports analog movement/look, LT aim, RT fire/blind fire, contextual A cover/roll/hold-run, X reload, Y weapon swap, B melee, stick-click crouch/sprint, explicit LB roll, RB shoulder swap and D-pad weapon/jump shortcuts. Stick dead zones, a finer look curve and reduced ADS sensitivity improve controller precision. HUD hints follow the last meaningful controller or keyboard/mouse input. Existing keyboard bindings and original input assets are preserved. See [CONTROLLER_TESTING](CONTROLLER_TESTING.md) for setup and the full layout.

Editor and Game builds and the fresh saved-input gate pass. Four completed rendered sessions pass 276 checks with zero failures: 90 controller checks and 186 keyboard/mouse regression checks. Final build evidence and the excluded initial probe-timing failures are recorded in TEST_MATRIX. Automated input cannot certify a physical controller's Bluetooth connection or in-hand feel. No aim assist, rumble, remapping menu or new animation feature is included; movement work stops here as requested.

---

# Cover movement extension — 2026-09-20

Cover entry now uses a checked, swept approach with supported-floor validation, dynamic-obstruction cancellation and safe arrival. The optional local profile adds Space tap-roll/hold-sprint, faster wall travel, bounded sprint turning with free mouse look, genuine directional crouched ADS, a hunched rifle sprint and rifle high-wall idle/lateral motion. Rifle grip correction follows the evaluated pose. Rolls can start from clear crouch/cover, preserve cover on rejected wall routes and resume held ADS afterward.

Eleven official Mixamo candidates and compatible installed Epic crouch/vault copies were prepared locally. Four Mixamo clips and seven installed Epic crouch clips are selected: idle, four directions, entry and exit. The seven Mixamo transition/vault candidates and installed vault remain dormant. The public Blueprint stays portable, with licensed source/derived assets and profile configuration ignored by Git. No paid pack was purchased. See LOCAL_MOVEMENT_SETUP for reproduction.

An earlier local graph completed 24 traversal scenarios in each of two rendered PIE sessions with zero failures/skips. A later editor restart auto-reimported 23 assets and corrupted the target poses, so that earlier run is excluded from final acceptance. The affected tree is preserved; FBX source storage/import metadata have been repaired, and a fresh recovery gate passed 2,013 poses across eleven clips and fourteen package hashes. Optional stationary free/high-cover crouch entry/exit and arm-only weapon carry are implemented, preserving authored torso/legs with immediate low-cover bypass and interruption by movement/aim/actions. The recovered profile passed 612 traversal, 186 movement, 68 blind-fire, 224 crouched-reload and 242 polish checks; portable fallback added 184 passes. Total: **1,516 checks across twelve rendered PIE sessions, zero failures**, including 30 traversal scenarios per session. Post-normal-editor persistence revalidated all 2,013 poses and fourteen package hashes with zero errors/warnings. Evidence and limitations are recorded in TEST_MATRIX. This does not complete the requested movement system: braced knee slides, corners/transfers, attached vaults, pistol wall poses and knife handling remain outstanding. The original foundation and deferred horde/co-op scope are preserved.

---

# Motion and weapon handling polish — 2026-09-20

Connected existing licensed motion for rifle/pistol jump takeoff and additive landing recovery, empty-trigger animation, protected crouched weapon switching, and a second standing melee attack. Jump clips are project-owned root-locked copies; CharacterMovement owns travel. Empty trigger causes no shot/ammo/damage and yields immediately to reload. F alternates jab/cross on accepted presses with one guarded impact per attack.

Weapon switching now works in free crouch and low cover. Existing equip arms compose over genuine crouch; switching from low-cover ADS lowers the character until the handling action finishes, then held ADS resumes. The previous graph and authoring backups remain recoverable. No source pack was acquired and no motion tracks were manually keyed. Static weapon models, dedicated cover/vault/knife gaps and the deferred horde/co-op scope remain as documented. Editor and Game builds succeed. Eight completed rendered PIE sessions pass 718 checks with zero failures; see TEST_MATRIX for actual evidence and limits.

---

# 0.3.1 — Reload while crouched

R now reloads the rifle or pistol while crouched, including behind low cover. Existing reload clips drive only the arms over the real crouch pose. Reload stops blind fire and temporarily lowers low-cover ADS; held ADS resumes after completion. Ammo still transfers only on uninterrupted completion, and stance toggles are disabled during reload.

The new animation graph preserves both previous graphs and reuses existing licensed clips. Weapon switching remains standing-only outside low cover; static weapon magazines/bolts remain provisional. Editor and Game builds succeed; six rendered PIE sessions pass 476 live checks, including 224 focused crouched-reload checks. Real gameplay captures were inspected. See TEST_MATRIX for evidence and limits.

# 0.3.0 — Crouched low-cover blind fire

LMB without ADS now raises the rifle or pistol above low cover while keeping the character crouched and the head hidden. The first shot waits for the actual evaluated weapon pose and clear path; rifle fire repeats while held, pistol fire stays one shot per press, and quick clicks queue one shot. RMB retains standing pop-up ADS. Blind fire uses a wider spread and the real muzzle; intervening walls still block damage.

The new animation graph combines the licensed crouch/weapon clips with native arm IK and preserves the previous graph. No replacement animation tracks were manually keyed and no new pack was purchased. Useful omitted animations—jump phases, dry fire, melee variants and future life-state motions—are now itemized in ANIMATION_BACKLOG instead of being conflated with missing sources. Editor and Game builds succeed; two blind-fire PIE sessions pass 68 checks and two broader motion sessions pass 184 checks. See TEST_MATRIX for rendered gameplay and visual evidence, including initial issues found and corrected.

# 0.2.1 — Mac mouse-input compatibility

Select Unreal's AppKit mouse-input path on macOS to address the reported loss of free look after releasing ADS. The change takes effect after restarting Unreal. Physical-mouse confirmation remains pending; automated checks cannot reproduce native hardware delivery.

Editor and Game targets build. Two rendered PIE sessions pass 98 look checks each across rifle/pistol aim and release, crouch, low cover and detach. The checks verify controller yaw/pitch, actual camera rotation and look-enabled state using synthetic Enhanced Input. Startup logs confirm the intended Mac backend setting is active. See TEST_MATRIX for evidence and limitations.

# 0.2.0 — Movement and weapon sandbox

Added a separate default movement range with imported rifle/pistol meshes, armed directional locomotion, shoulder ADS and shoulder swapping, firing/reload/equip actions, real retargeted crouch and sprint, a guarded dodge roll, and a standing melee jab. Static low/high cover supports attachment, movement along walls, low-cover pop-up aim, and checked physical exposure/return at high-cover edges. Targets and an owning-player HUD make weapon behavior visible.

Native components own action guards, ammo, obstruction checks and movement; Blueprint/Data Assets compose the acquired Epic template and Quaternius CC0 content. Editable sources, licenses, IK retarget setup and guarded authoring tools are retained. Character appearance and the horde game remain deferred; the original foundation map is preserved.

Validation: Editor and Game targets build. Two rendered PIE sessions pass 92 live checks each, including evaluated animation slots, actual target damage, cover return, roll collision/interruption, reload conservation and incompatible action guards. Real gameplay screenshots were inspected. See TEST_MATRIX for evidence and engine/environment warnings.

Remaining motion gaps: dedicated cover transitions/lean/corners, vault, knife-specific handling, and directional crouched ADS. Current melee is a jab. This is a playable prototype, not a complete Gears-style animation set or packaged release.

# 0.1.0 — Unreal foundation

Project Gunner now starts from a clean UE 5.8.2 C++/Blueprint project. The README-only Godot remote was backed up, deleted and recreated under the same GitHub name.

Implemented: saved metric test map, composed Manny pawn/Animation Blueprint, collision-aware shoulder camera, native character movement and grounded jump, Enhanced Input actions/data asset and possession-safe local mapping ownership. Added opt-in live smoke diagnostics and build/run scripts.

Documentation: Unreal-specific AGENTS, technical architecture, system-by-system migration, milestone gates, humanoid animation strategy, updated art pipeline/content/provenance records and untouched historical references.

Validation: Editor and Game targets compile; real game launch and visual captures; 20 live checks pass in each of two PIE sessions. See TEST_MATRIX for exact evidence, environment warnings and limitations.

Next authorized milestone should establish proper crouched locomotion, aiming/sprint and the retargeting/weapon animation contract before expanding gameplay. This release is not yet the horde vertical slice.
