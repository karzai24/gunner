# Unreal art and technical production standards

Preserve the original visual direction in `ART_DIRECTION.md`: original solar-industrial equipment, readable Warden silhouettes, crystalline Veyra and clear tactical spaces. The complete prior art bible is retained at `reference/godot/ART_BIBLE.md`; its appearance, deformation, UV, source-preservation and readability goals remain useful, while this document replaces its Godot-specific paths, axes and tooling.

## Production baseline

Use `Content/Gunner` for authored runtime assets; `Content/Characters` preserves Epic template package paths. Store editable DCC sources under `ArtSource/<family>` when introduced. Never use a cooked/export-only file as the sole editable source. Retain recoverable sources before destructive edits. No final character art until the animation contract passes.

Runtime units are centimeters, Z up and +X forward. Blender may author in meters, but validate a single explicit unit/axis conversion on export with a 1 m cube and the canonical skeleton. Do not compensate scale at multiple runtime layers. Prefer a verified FBX skeletal animation pipeline into Unreal; evaluate Interchange/glTF for static geometry only where useful. The old Windows Blender/MCP setup is historical and has not been installed or validated on this Mac.

Use Unreal prefixes: `BP_`, `ABP_`, `SKM_`, `SK_`, `SM_`, `M_`, `MI_`, `T_`, `IA_`, `IMC_`, `DA_`, `L_`, `IK_`, `RTG_`, `AM_`. Keep incoming template names stable. Meaningful snake_case is appropriate for DCC source files. Imported assets are composed in Blueprint children; do not put unrelated gameplay in imports.

## Geometry and materials

Starting hero budget: approximately 45–65k triangles at LOD0, LOD1 50–60%, LOD2 20–30%; use less when silhouette/deformation remain sound. Start at 2K hero textures, 1–2K enemies/weapons and shared tiling/trim environments. These are planning budgets, not observed performance. Minimize material slots, overdraw and redundant textures. Use instances for variation; document ORM channel packing (R occlusion, G roughness, B metallic), sRGB and normal map conventions.

Preserve believable joint deformation, normalized weights, compatible influences, intentional UV padding and pivots. Validate extreme crouch/aim/vault poses, shoulder/hip volume, clothing separation and hair/gear clipping. Prefer controllable hair masses/cards; no costly strand pipeline without profiling.

Simple primitives/convex collision are preferred. Use ISM/HISM for appropriate repeated static content, not Godot MultiMesh. Do not change cover/lane metrics during dressing without retesting. Use LODs and deliberate shadow/visibility choices. Color reinforces shape/value/timing cues and must not be the sole gameplay indicator.

## Integration gate

Verify actual Unreal import settings, skeleton/root policy, looping, textures/materials, collision and sockets. Inspect from the real shoulder camera and, once implemented, both split-screen views. Check idle/moving/crouching/aiming/cover/vault/death as applicable. Build, launch and profile representative use. A concept, DCC render or successful import is insufficient. Record provenance and known limitations; no unverified third-party license assets.
