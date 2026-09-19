# Project Gunner — Art Bible and Technical Art Standards

This document defines how Project Gunner visual assets are created, named, organized, optimized, exported, imported, tested, and maintained.

`docs/ART_DIRECTION.md` defines what the game should look and feel like. This file defines how to produce that look reliably.

Before changing an established asset pipeline, inspect the repository and confirm the current working conventions. Verified project conventions take precedence over the defaults in this document unless they conflict with `AGENTS.md`, licensing rules, originality requirements, or the user's current request.

---

## 1. Production Principles

All assets should be:

- Original or properly licensed.
- Editable at the source.
- Appropriate for the gameplay camera.
- Optimized for the target scene and split-screen use.
- Organized predictably.
- Imported without avoidable warnings.
- Tested inside Godot.
- Documented when they establish a reusable convention.

Prefer a coherent, efficient asset over an unnecessarily complex one.

Do not spend geometry, texture memory, material slots, or shader cost on detail that cannot be seen during normal play.

---

## 2. Source of Truth

For each asset, preserve:

1. Editable source file.
2. Exported runtime file.
3. Textures and dependent resources.
4. Godot scene or material setup.
5. Asset-specific brief when the design is important or reusable.
6. Provenance and license record when any external source is used.

Do not treat an exported `.glb` as the only editable source for a custom asset.

Do not overwrite a user-authored source file without keeping a recoverable version.

---

## 3. Recommended Repository Organization

Use the project's established structure when present. Otherwise, prefer:

```text
res://
|-- assets/
|   |-- source/
|   |   |-- blender/
|   |   |   |-- characters/
|   |   |   |-- enemies/
|   |   |   |-- environments/
|   |   |   |-- weapons/
|   |   |   `-- props/
|   |   `-- textures/
|   |-- characters/
|   |-- enemies/
|   |-- environments/
|   |-- weapons/
|   |-- props/
|   |-- animation/
|   |-- vfx/
|   `-- ui/
|-- scenes/
|-- shaders/
`-- docs/
    |-- characters/
    |-- enemies/
    |-- environments/
    `-- assets/
```

Source files may live outside `res://` if the repository intentionally excludes large authoring files from Godot imports. Follow the established project convention and document the location.

Avoid duplicate exports in multiple folders.

---

## 4. Naming Standards

Use lowercase `snake_case` for asset files unless an existing pipeline requires otherwise.

Examples:

```text
hero_a.blend
hero_a.glb
hero_a_body_basecolor.png
hero_a_body_normal.png
hero_a_body_orm.png
alien_breacher_lod0.glb
env_industrial_wall_4m_a.glb
prop_barricade_foldout_a.glb
wpn_rifle_service_a.glb
```

Use meaningful prefixes only when they improve grouping.

Suggested prefixes:

- `chr_` — character.
- `enm_` — enemy.
- `env_` — environment.
- `prop_` — prop.
- `wpn_` — weapon.
- `vfx_` — visual effect.
- `ui_` — interface.
- `mat_` — material.
- `tex_` — texture.
- `anim_` — animation.
- `skel_` — skeleton.
- `col_` — collision.
- `sock_` — attachment socket.

Do not use names such as:

- `final`.
- `final2`.
- `new`.
- `test123`.
- `copy`.
- `untitled`.
- `mesh001`.

Blender objects, materials, collections, bones, animation actions, and Godot resources must have stable descriptive names before delivery.

---

## 5. Units, Scale, Axes, and Transforms

Project baseline:

- 1 Godot unit = 1 meter.
- Positive Y is up in Godot.
- Character scale must match the existing controller and collision setup.
- Forward direction must be consistent across the active character and animation pipeline.

Before modeling a new character or prop:

1. Inspect an existing correctly scaled scene.
2. Confirm the expected forward axis.
3. Confirm origin and pivot requirements.
4. Confirm whether root motion is used.
5. Confirm export settings already proven in the repository.

General Blender requirements:

- Use real-world meter scale.
- Apply scale before final export unless the verified rig pipeline requires otherwise.
- Avoid negative object scale.
- Resolve unintended rotation and scale values.
- Keep armature and mesh transforms compatible.
- Place origins intentionally.
- Remove hidden duplicate objects and unused collections.
- Check face orientation and normals.

Do not repair an import problem by adding arbitrary scale compensation in multiple Godot nodes unless the source pipeline cannot be changed safely.

---

## 6. Blender File Standards

Each production `.blend` should be understandable without the original creator present.

Recommended top-level collections:

```text
00_reference
10_model
20_high
30_low
40_rig
50_collision
60_export
90_helpers
```

Only keep collections that are useful for the asset.

Before delivery:

- Name every production object.
- Remove unused meshes, materials, images, actions, and collections.
- Purge orphaned data when safe.
- Keep references separated from export objects.
- Disable or remove viewport-only modifiers that should not export.
- Confirm modifier order.
- Apply modifiers only when doing so is appropriate and recoverable.
- Keep an editable pre-application version when destructive changes are significant.
- Confirm no external file path is broken.
- Pack or collect source dependencies when required by the repository workflow.

The export collection should contain only intended runtime objects.

---

## 7. Character Scale and Geometry Budgets

Use existing measured project budgets when available.

Reasonable starting targets for a primary playable character:

- LOD0: approximately 45,000–65,000 triangles including clothing and hair.
- LOD1: approximately 50–60% of LOD0.
- LOD2: approximately 20–30% of LOD0.

These are guidelines, not mandatory quotas.

Lower counts are preferred when silhouette, deformation, and close-camera quality remain strong.

Spend geometry on:

- Face silhouette.
- Hair silhouette.
- Hands.
- Shoulders.
- Elbows.
- Hips.
- Knees.
- Boots.
- Clothing folds that affect the outline.
- Equipment visible from the gameplay camera.

Reduce geometry on:

- Hidden body surfaces beneath clothing.
- Flat interior surfaces.
- Tiny buckles and seams.
- Unseen equipment backs.
- Details better represented in normal maps.
- Areas never viewed closely.

Avoid overlapping hidden body geometry that causes clipping during animation.

---

## 8. Character Topology

Character topology must support the animations required by the game.

Prioritize clean deformation around:

- Shoulders.
- Armpits.
- Elbows.
- Wrists.
- Fingers when animated.
- Neck.
- Hips.
- Groin.
- Knees.
- Ankles.
- Mouth and eyes when facial movement is required.

Use:

- Continuous edge flow around major joints.
- Enough loops to preserve volume.
- Controlled poles away from high-deformation zones.
- Separate clothing meshes when they simplify iteration.
- Hidden-body removal when clothing is permanent and clipping risk is high.

Avoid:

- Dense sculpt topology as the runtime mesh.
- Long thin triangles at joints.
- Unnecessary subdivisions.
- Nonmanifold geometry.
- Accidental internal faces.
- Unweighted vertices.
- Duplicate vertices or faces.
- Uncontrolled overlapping surfaces.

Validate with extreme representative poses, not only an A-pose.

---

## 9. Character Heads, Skin, and Hair

### Head

A hero head should remain recognizable from:

- Front.
- Three-quarter.
- Profile.
- Normal gameplay camera distance.

Use asymmetry carefully.

Do not rely on pores or micro-wrinkles to create identity.

### Eyes

Eyes must:

- Sit correctly in the sockets.
- Avoid visible clipping.
- Maintain stable orientation.
- Use a restrained, performant material setup.
- Avoid excessive wetness or emissive appearance.

### Teeth and Mouth

Only include the complexity required by visible animation.

Do not add full dental or tongue geometry when the character has no close facial animation.

### Hair

For the project's grounded stylized realism:

- Design the large hair mass first.
- Build a readable outer silhouette.
- Use cards, modeled clumps, or a hybrid approach based on the renderer and target hardware.
- Keep transparency overdraw controlled.
- Avoid expensive strand systems in runtime exports unless profiling proves them acceptable.
- Test hair under gameplay lighting and camera motion.
- Ensure hair does not reproduce a recognizable existing character silhouette.

---

## 10. Clothing and Equipment

Clothing construction should be readable and plausible.

Use major layers:

1. Base layer.
2. Main garment.
3. Reinforcement or armor.
4. Harness or utility system.
5. Equipment.

Model only the folds that matter to silhouette, construction, or deformation.

Use normal maps for smaller folds and stitching.

Equipment must:

- Have a plausible attachment.
- Avoid floating or penetrating the body in common poses.
- Preserve shoulder and arm movement.
- Avoid obstructing the camera.
- Avoid excessive secondary motion in multiplayer and split-screen.
- Remain distinguishable from adjacent clothing.

Test:

- Sprint.
- Crouch.
- Aim.
- Cover idle.
- Cover movement.
- Vault.
- Death or incapacitated pose.
- Weapon switching when available.

---

## 11. Skeleton and Rig Standards

Inspect the existing player skeleton and animation library before creating a new rig.

Prefer one reusable humanoid skeleton for playable characters whenever practical.

A character rig must document:

- Skeleton source.
- Bone naming.
- Bone orientation.
- Rest pose.
- Root bone.
- Root motion policy.
- Retargeting process.
- Attachment bones or sockets.
- Exported versus control bones.
- Any required import settings.

Do not export control rigs, IK helpers, reference bones, or constraints unless the runtime pipeline explicitly needs them.

Suggested attachment points:

- Right hand weapon.
- Left hand support reference when required.
- Sidearm.
- Back weapon.
- Throwable or tool.
- Head.
- Chest.
- Pelvis.
- Muzzle on weapon assets.
- Optional VFX points.

Use stable names and document them.

---

## 12. Skinning and Weighting

Every skinned asset must be checked for:

- Unweighted vertices.
- Unexpected bone influence.
- Excessive influence counts.
- Volume collapse.
- Candy-wrapper twisting.
- Shoulder pinching.
- Elbow and knee spikes.
- Clothing-body separation.
- Equipment drift.
- Hair clipping.
- Boot and hand deformation.

Limit influences according to the active Godot and import pipeline.

Normalize weights.

Use corrective shapes only when they provide a visible benefit and are supported reliably by the runtime pipeline.

Do not hide major weighting failures with camera framing.

---

## 13. Animation Standards

Animation must serve gameplay state and readable transitions.

Use the existing animation naming and organization when present.

Common categories:

- Idle.
- Walk.
- Run.
- Sprint or roadie run.
- Aim locomotion.
- Crouch.
- Cover enter.
- Cover idle.
- Cover movement.
- Cover exit.
- Peek.
- Corner transition.
- Vault or mantle.
- Fire.
- Reload.
- Melee.
- Hit reaction.
- Stagger.
- Death.
- Interaction.

Requirements:

- Gameplay authority does not depend only on animation timing.
- Animation events may request gameplay commits, but code validates them.
- Root motion policy is explicit.
- Foot sliding is minimized.
- Hands align with weapon grip points.
- Muzzle and projectile origins agree.
- Cover poses align with gameplay metrics.
- Missing clips use a safe fallback rather than soft-locking the actor.
- Transitions are tested from the actual controller.

Do not create a large animation library before the controller and skeleton contracts are known.

---

## 14. UV Standards

Use intentional, non-overlapping UVs for unique texture areas unless mirroring or overlap is deliberately chosen.

Requirements:

- Consistent texel density within an asset family.
- Sufficient padding for mipmaps.
- Straightened UVs where useful for hard-surface and trim workflows.
- Mirroring only where asymmetry is not important.
- Separate lightmap UV channel when required by the active lighting pipeline.
- No accidental stacked islands.
- No islands outside expected tile space unless UDIMs are deliberately supported.
- No severe stretching in visible areas.

Prioritize face, hands, upper torso, weapon contact areas, and character-specific details.

---

## 15. Texture Standards

Use the smallest texture size that preserves the intended gameplay result.

Suggested starting points:

### Hero Character

- 2K body/clothing set.
- Optional separate 2K head set when justified.
- 1K or 2K equipment set depending on screen coverage.
- Avoid 4K by default.

### Standard Enemy

- 1K or 2K depending on size and encounter count.

### Weapons

- 1K or 2K depending on camera distance and whether first-person inspection exists.

### Environment Modules

- Prefer shared tiling textures, trim sheets, atlases, and decals.
- Use unique textures only for hero props or special storytelling assets.

### Small Props

- 512 or 1K where sufficient.
- Atlas repeated props when practical.

Texture names should indicate material family and channel.

Example:

```text
hero_a_body_basecolor.png
hero_a_body_normal.png
hero_a_body_orm.png
hero_a_body_emission.png
```

When compatible with the current Godot material setup, pack:

- Ambient occlusion.
- Roughness.
- Metallic.

Document the channel order and use it consistently.

---

## 16. Material Standards

Minimize material count.

A hero character should normally use only the slots needed to support clear material behavior, such as:

- Skin.
- Eyes.
- Hair.
- Main clothing.
- Equipment or metal.

Combine materials when this does not harm quality or iteration.

Avoid creating a separate material for every small object.

Use material instances for variation.

Materials must be checked for:

- Correct color space.
- Normal map import.
- Roughness response.
- Metallic response.
- Transparency mode.
- Double-sided rendering.
- Shadow behavior.
- Emission.
- Texture filtering.
- Mipmap behavior.
- Renderer compatibility.

Do not use expensive shader features without testing their cost in representative gameplay.

---

## 17. Environment Modular Standards

Environment art must preserve gameplay metrics established by level design.

Use a modular grid appropriate to the current project.

A practical starting family may include:

- 0.5 m detail increments.
- 1 m structural increments.
- 2 m and 4 m wall modules.
- Standardized floor and ceiling heights.
- Standard cover heights defined in `docs/LEVEL_DESIGN.md`.

Do not assume these dimensions if the project already has verified metrics.

Each module should have:

- Intentional pivot.
- Clean snapping behavior.
- Matching edges.
- Consistent texel density.
- Separate simple collision when required.
- Material compatibility with the kit.
- Clear interior and exterior use.
- LOD or visibility strategy when repeated heavily.

Avoid unique one-off geometry for spaces that should be built from the modular kit.

---

## 18. Cover and Traversal Art Standards

Art must not break validated cover and traversal behavior.

For every cover asset:

- Confirm cover height.
- Confirm thickness.
- Confirm edge clearance.
- Confirm player camera clearance.
- Confirm muzzle behavior.
- Confirm peek and corner readability.
- Confirm vaultability where intended.
- Confirm collision simplicity.
- Confirm navigation impact.
- Confirm defensive-object interaction when relevant.

Decorative pieces must not create invisible collision surprises.

Do not change a greybox metric during the art pass without retesting gameplay.

---

## 19. Collision Standards

Use the simplest collision that correctly supports gameplay.

Prefer:

- Primitive collision.
- Convex collision.
- Purpose-built low-poly collision meshes.
- Separate collision objects from render geometry.

Avoid full detailed trimesh collision for dynamic objects.

Collision requirements:

- No invisible protrusions.
- No gaps that trap the player.
- No collision on purely decorative details unless required.
- Stable floor and stair contact.
- Correct layer and mask assignment.
- Clear distinction between player, projectile, interaction, and navigation needs.
- Defensive objects must reflect their actual blocking state.

Name collision objects clearly.

---

## 20. LOD, Visibility, and Performance

Use LODs or visibility strategies for:

- Characters.
- Enemies.
- Large props.
- Repeated environment assets.
- Expensive transparent meshes.
- Dense distant geometry.

Test LOD transitions from gameplay camera distances.

Avoid large silhouette pops.

Performance priorities:

1. Reduce unnecessary material slots.
2. Reduce transparent overdraw.
3. Reduce shadow-casting cost.
4. Reduce repeated unique textures.
5. Reduce unseen geometry.
6. Use shared meshes and materials.
7. Use MultiMesh for suitable repeated static or simple assets.
8. Use occlusion and visibility ranges where proven useful.
9. Profile before introducing complex optimization systems.

Split-screen can multiply rendering cost. Validate representative scenes with the number of local views the milestone supports.

---

## 21. Alien Asset Standards

Alien assets should share a coherent construction pipeline.

Document for each enemy family:

- Shared skeleton or skeleton family.
- Reusable materials.
- Armor and tissue rules.
- Damage states.
- Weak points.
- VFX attachment points.
- Attack origin points.
- Hitbox regions.
- LOD strategy.
- Corpse or disappearance policy.

Enemy silhouettes must remain distinct at the intended encounter distance.

Do not make every alien use a unique skeleton unless their anatomy truly requires it.

For horde enemies, prioritize:

- Efficient rigs.
- Limited material slots.
- Controlled transparency.
- Reusable animation.
- Clear attack anticipation.
- Stable navigation footprint.
- Bounded corpse and effect cost.

---

## 22. Weapon Asset Standards

Weapons must include:

- Correct scale.
- Stable origin.
- Grip references.
- Muzzle point.
- Optional shell ejection point.
- Magazine or reload references where needed.
- Sight or aim reference where needed.
- Collision or interaction proxy where needed.
- Documented attachment points.
- Appropriate LOD or visibility behavior.

Test weapons with:

- Idle.
- Aim.
- Fire.
- Reload.
- Cover.
- Shoulder swap if supported.
- Character A and Character B proportions.

Do not finalize weapon placement from Blender alone.

---

## 23. VFX Technical Standards

Transient VFX should be pooled or bounded when used frequently.

For each gameplay VFX family, define:

- Trigger.
- Lifetime.
- Maximum simultaneous count.
- Fallback when exhausted.
- Color and value language.
- Light usage.
- Decal usage.
- Sound relationship.
- Accessibility reduction behavior where relevant.

Avoid:

- Long-lived transparent particles.
- Unbounded decals.
- Excessive dynamic lights.
- Full-screen flashes.
- Effects that obscure the reticle or target.
- High-frequency particles in split-screen without profiling.

---

## 24. UI Art Standards

UI assets must support:

- Mouse.
- Keyboard.
- Controller.
- Split-screen readability.
- UI scaling.
- Multiple aspect ratios.
- Localization expansion.
- Clear focus and disabled states.
- Color-independent communication.

Use vector or scalable source formats where practical.

Avoid baking gameplay-critical text into images.

Keep icons readable at their smallest intended display size.

---

## 25. Export Standards

Use the project's proven export format and settings.

For Godot, glTF/GLB is the default unless the existing project uses another verified pipeline.

Before export:

- Save the `.blend`.
- Confirm units.
- Confirm forward direction.
- Confirm transforms.
- Confirm export collection.
- Confirm object and material names.
- Confirm skeleton and animation selection.
- Confirm texture paths.
- Remove temporary objects.
- Check normals.
- Check modifiers.
- Check hidden geometry.
- Check LOD names.
- Check attachment nodes.

After export:

- Import into Godot.
- Review importer warnings.
- Confirm materials.
- Confirm texture channels.
- Confirm skeleton.
- Confirm animation names.
- Confirm scale and orientation.
- Confirm origin and floor contact.
- Confirm attachment points.
- Confirm collision.
- Confirm runtime performance.

A successful export is not completion.

---

## 26. Godot Import Standards

Do not accept default import settings blindly.

Inspect and configure:

- Mesh compression.
- Normal and tangent generation.
- LOD generation or imported LODs.
- Material import behavior.
- Animation import.
- Loop settings.
- Root motion.
- Skeleton rest.
- Bone mapping.
- Skin influence limits.
- Collision generation.
- Texture compression.
- Normal map detection.
- Mipmaps.
- Filtering.
- sRGB behavior.
- Transparency.
- Repeat mode.

Prefer explicit reusable import workflows over repeatedly fixing individual assets manually.

Do not modify `.godot/imported/` files directly.

---

## 27. Godot Scene Integration

A production asset should normally be wrapped in a Godot scene that defines its runtime contract.

A character scene may include:

- Imported model.
- Skeleton.
- AnimationTree.
- Collision body.
- Hitboxes.
- Interaction component.
- Weapon sockets.
- VFX points.
- Audio points.
- Character-specific material overrides.
- Debug validation helpers gated from release.

Do not embed unrelated gameplay logic in an imported model scene when a clean wrapper scene is more maintainable.

Preserve the ability to reimport the source model without losing runtime setup.

---

## 28. Asset Validation

Every production asset must be validated at the level appropriate to its use.

### Visual

- Silhouette reads from gameplay camera.
- Materials respond correctly.
- No obvious clipping.
- No broken normals.
- No z-fighting.
- No unintended transparency.
- No visible texture seams.
- No missing textures.
- Wear and detail are logically placed.
- Asset matches `docs/ART_DIRECTION.md`.

### Technical

- Correct scale.
- Correct orientation.
- Correct pivot.
- Correct naming.
- Correct material count.
- Correct texture size.
- No unused objects or materials.
- No import errors.
- No broken paths.
- No invalid skeleton or animation tracks.
- Collision and navigation behavior are correct.

### Gameplay

- Asset is used in the real scene.
- Camera framing works.
- Cover and muzzle behavior work where applicable.
- Animation deformation works where applicable.
- Split-screen performance is acceptable where relevant.
- Interaction is readable.
- Death, reset, reload, or scene transition does not break it.

---

## 29. Asset Documentation Template

For important assets, create a brief under the relevant `docs/` directory.

Use this structure:

```markdown
# Asset Name

## Purpose

## Visual Concept

## Gameplay Role

## Originality Notes

## Scale and Dimensions

## Source Files

## Runtime Files

## Materials and Textures

## Skeleton and Animation

## Collision and Attachments

## Triangle and Material Counts

## Godot Scene

## Validation Performed

## Known Limitations

## Decisions Future Agents Must Preserve
```

Character briefs should also describe:

- Face.
- Hair.
- Body proportions.
- Clothing construction.
- Palette.
- Distinguishing silhouette.
- Multiplayer readability.

---

## 30. Asset Provenance

Record every external asset, texture, brush, scan, audio source, font, or kit in `docs/ASSET_PROVENANCE.md`.

Include:

- Asset name.
- Original creator.
- Source.
- License.
- Commercial-use status.
- Modification rights.
- Date obtained.
- Local file location.
- Modifications made.
- Attribution requirement.

Do not integrate an external asset when the license is unknown or incompatible.

Generated assets must also record the tool and relevant usage constraints when required by the project.

---

## 31. Blender MCP Operating Rules

When Blender MCP is available and Blender work is requested:

- Use it to perform the actual modeling, UV, material, rigging, animation, and export work.
- Inspect the existing Blender and Godot pipelines first.
- Save editable source files.
- Keep changes recoverable.
- Do not substitute a written tutorial for requested asset creation.
- Do not claim an operation succeeded without inspecting the Blender result.
- Export and validate in Godot.
- Report the exact blocker when the MCP, Blender instance, exporter, or import pipeline fails.

Do not use Blender MCP to destructively rewrite unrelated user-authored assets.

---

## 32. Completion Criteria

An art task is complete only when all applicable conditions are true:

- The design follows `docs/ART_DIRECTION.md`.
- The asset is original.
- The editable source exists.
- Naming and organization are clean.
- Geometry is appropriate for its use.
- UVs and materials are complete.
- Rigging and animation compatibility are validated when applicable.
- Runtime export exists.
- Godot import is configured.
- The asset is connected to its intended scene or flow.
- Scale, orientation, collision, materials, and attachments are correct.
- The asset was inspected from the actual gameplay camera.
- Relevant performance was checked.
- Documentation and provenance are current.
- Known limitations are reported honestly.

A concept, render, viewport screenshot, model file, export, or successful import alone does not satisfy completion.
