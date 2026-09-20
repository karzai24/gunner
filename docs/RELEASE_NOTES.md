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
