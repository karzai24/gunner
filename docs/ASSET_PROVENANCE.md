# Asset provenance — Unreal foundation

The historical ledger under `reference/godot/ASSET_PROVENANCE.md` is a record of the old prototype, not a list of files shipped here. No historical custom character, Mixamo, Quaternius or GaspFix files were available/imported in this rebuild.

| Asset | Creator/source/version | License and use | Location / modification |
|---|---|---|---|
| Manny/Quinn meshes, SK_Mannequin, materials/textures, Control Rigs, unarmed/rifle/pistol/death animation candidates | Epic Games, installed UE 5.8.2 `Templates/TemplateResources/High/Characters/Content`, copied 2026-09-19 | Unreal Engine EULA Examples; used as temporary game development content, not project-owned art or CC0 | `Content/Characters/Mannequins`; copied unchanged; BP_Warden references Manny Simple + ABP_Unarmed |
| Cube and default engine material dependencies | Epic Games UE 5.8.2 Engine Content | Unreal Engine licensed technology | Referenced through `/Engine/BasicShapes/Cube`; not copied or redistributed as engine source |
| Foundation map/materials/input assets/Blueprint composition/C++/scripts | Authored for Project Gunner in this task with Codex | Project source; no third-party open-source license assigned | `Content/Gunner`, `Source/Gunner`, `Tools`; all new |
| Supplied design and historical documents | Project owner, supplied Downloads/docs | Preserved project documentation | `docs/reference/godot`; originals retained without edits |

Official source: [Unreal Engine EULA](https://www.unrealengine.com/eula/unreal), checked 2026-09-19. Section 1 defines content in installed Samples/Templates as Examples; section 5(b) permits distribution of Examples in source or object code. Retain Epic's ownership and applicable notices; do not relicense these as original Project Gunner content. The game's eventual credits/distribution must satisfy applicable Unreal notices. No new marketplace asset, purchase or license acceptance was performed in this task.

The installed character pack includes future animation candidates; presence is not validation. `ANIMATION_PIPELINE.md` distinguishes integrated standing locomotion from missing/unevaluated actions. If replacing the pack later, record file hashes/source version, license and retarget changes before use. Final Micah/Rook art must preserve original identity and avoid recognizable franchise designs.
