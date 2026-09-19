# 0.1.0 — Unreal foundation

Project Gunner now starts from a clean UE 5.8.2 C++/Blueprint project. The README-only Godot remote was backed up, deleted and recreated under the same GitHub name.

Implemented: saved metric test map, composed Manny pawn/Animation Blueprint, collision-aware shoulder camera, native character movement and grounded jump, Enhanced Input actions/data asset and possession-safe local mapping ownership. Added opt-in live smoke diagnostics and build/run scripts.

Documentation: Unreal-specific AGENTS, technical architecture, system-by-system migration, milestone gates, humanoid animation strategy, updated art pipeline/content/provenance records and untouched historical references.

Validation: Editor and Game targets compile; real game launch and visual captures; 20 live checks pass in each of two PIE sessions. See TEST_MATRIX for exact evidence, environment warnings and limitations.

Next authorized milestone should establish proper crouched locomotion, aiming/sprint and the retargeting/weapon animation contract before expanding gameplay. This release is not yet the horde vertical slice.
