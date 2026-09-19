# Asset Provenance

This record covers external source material present in, or used to generate, Project Gunner assets. Game-specific compositions, code, UI, weapons, enemies, environment geometry, effects, synthesized audio, and the Project Gunner modifications to Hero A were authored for this repository by OpenAI Codex on 2026-08-01 unless noted below.

## Hero A / Micah Vale

Hero A uses the official free MakeHuman/MPFB pipeline as a human-topology foundation. It does not contain a downloaded franchise character, facial scan, copyrighted costume, third-party animation library, or user-reference image texture.

| Item | Source and version | License/status | Project use and location |
|---|---|---|---|
| MPFB Blender extension | MPFB 2.0.17 from the official [Blender Extensions listing](https://extensions.blender.org/add-ons/mpfb/) and [MPFB documentation](https://static.makehumancommunity.org/mpfb/docs/getting_started.html) | GPL authoring extension; installed locally and not redistributed in this repository | Used through Blender MCP to create/finalize the human source, load system assets, and preserve editable inputs in `assets/source/characters/hero_a.blend` |
| MakeHuman System Assets pack | Official [MakeHuman System Assets](https://static.makehumancommunity.org/assets/assetpacks/makehuman_system_assets.html), downloaded 2026-08-01 from `files2.makehumancommunity.org`; ZIP SHA-256 `B542127A8E25547C7C29C19F2D1D2ADB9A664C80396ECD694095DBC8028A0107` | CC0 / public domain according to the pack page and [MakeHuman license guidance](https://static.makehumancommunity.org/about/license.html) | Human base mesh, macro/detail morph targets, and MPFB fitting data embedded in the editable `.blend`; derived runtime geometry in `assets/characters/hero_a.glb` |
| Middle-age male skin | MakeHuman System Assets, middle-age Caucasian male skin | CC0 | Used as a restrained weathered surface base; copied/processed into `assets/characters/hero_a_textures/hero_a_skin_basecolor.png` and embedded in the GLB |
| Low-poly eyes | MakeHuman System Assets low-poly eyes | CC0 | Fitted, weighted, and exported as `hero_a_eyes_lod0`; project-local PBR maps in `assets/characters/hero_a_textures/` |
| `short03` hair | MakeHuman System Assets | CC0 | Current runtime hair topology; fitted, recolored muted chestnut, and reshaped into an asymmetric medium sweep; exported as `hero_a_hair_lod0` |
| `male_casualsuit04` clothing | MakeHuman System Assets | CC0 | Current runtime fitted short-sleeve top and trouser topology; stock logo/accent treatment removed, palette replaced with Project Gunner charcoal/blue-gray/brown values, and authored equipment added; exported as `hero_a_clothing_lod0` |
| `shoes03` footwear | MakeHuman System Assets | CC0 | Current runtime fitted work-boot topology, recolored as dark practical field boots and combined into the equipment surface |
| Preserved `short02`, `male_casualsuit05`, and `shoes01` inputs | MakeHuman System Assets | CC0 | Retained only as hidden editable fitting references and in the recoverable `assets/source/characters/hero_a_v3_mpfb_olive.blend`; not used by the current runtime meshes |

Project-authored Hero A work includes the selected macro profile, stored detail/asymmetry targets, eyebrow-to-upper-cheek scar, asymmetric hair reshaping, technical chest yoke, selective armor, unequal harness, belt, knee guards, thigh pocket, teal back relay, PBR conversion, UV/material organization, continuous hand correction, rig fitting, weight cleanup, socket integration, and Godot wrapper/AnimationTree integration.

The legacy Micah Vale actions were authored in the repository for `skel_hero_humanoid`. The current playable Character A uses the separately recorded Quaternius Standard animation library and mannequin listed below. The user's supplied reference images were viewed only for broad qualities such as human proportion and survival-action presence. Their pixels, geometry, face, exact hair, costume, equipment layout, logos, and animations were not incorporated.

## Other project assets

| Asset group | Source/tool | License/status | Location |
|---|---|---|---|
| Warden placeholder, Veyra, weapon, and arena meshes | Original procedural Godot mesh composition | Project-owned source | `core/utilities/visual_factory.gd`, scene scripts |
| Hero A project-specific model edits, equipment, rig integration, actions, material conversion, and texture processing | Original Blender 4.5 LTS production through Blender MCP, using only the CC0 inputs itemized above | Project-owned modifications over CC0 inputs | `assets/source/characters/hero_a.blend`, `assets/characters/hero_a.glb`, `assets/characters/hero_a_textures/`, `tools/blender/` |
| UI and icons | Original Godot Control/vector drawing | Project-owned source | `scenes/ui/` |
| Combat/UI ambience cues | Original procedural PCM synthesis | Project-owned source | `autoload/audio_manager.gd`, `default_bus_layout.tres` |
| Godot icon | Default project bootstrap icon | Godot Engine asset; replace before external release branding | `icon.svg` |

No third-party purchase, subscription, account creation, private-project upload, or unverified-license asset was used.

## Animation comparison candidates

| Item | Source and version | License/status | Project use and location |
|---|---|---|---|
| Quaternius Universal Animation Library Standard | [Godot Asset Store listing](https://store.godotengine.org/asset/quaternius/universal-animation-library/) and [Quaternius pack page](https://quaternius.com/packs/universalanimationlibrary2.html); downloaded 2026-08-01 | CC0 1.0 Universal according to the publisher listing; active runtime placeholder for Character A | Standard Godot GLB and source archive are preserved under `assets/animation_candidates/quaternius_standard/` and `assets/animation_candidates/quaternius_universal_animation_library_standard.zip`; active Character A wrapper references the GLB through `scenes/player/hero_a_character.tscn` |

| Quaternius Modular Sci-Fi MegaKit Standard | [Official pack page](https://quaternius.itch.io/modular-sci-fi-megakit); downloaded 2026-08-01 | CC0 1.0 Universal according to the publisher listing; staged environment candidate | Standard archive and extracted glTF assets are preserved under `assets/environment_candidates/quaternius/modular_sci_fi_megakit/`; not connected to the live arena |
| Quaternius Sci-Fi Essentials Kit Standard | [Official pack page](https://quaternius.itch.io/sci-fi-essentials-kit); downloaded 2026-08-01 | CC0 1.0 Universal according to the publisher listing; staged props, weapon, and enemy candidate | Standard archive and extracted assets are preserved under `assets/environment_candidates/quaternius/sci_fi_essentials_kit/`; not connected to live gameplay |
| Quaternius Universal Base Characters Standard | [Official pack page](https://quaternius.itch.io/universal-base-characters); downloaded 2026-08-01 | CC0 1.0 Universal according to the publisher listing; staged Character B/variant candidate | Standard archive and extracted Godot glTF assets are preserved under `assets/character_candidates/quaternius_universal_base/`; not connected to live gameplay |
| Quaternius Universal Animation Library 2 Standard | [Official pack page](https://quaternius.com/packs/universalanimationlibrary2.html); downloaded 2026-08-01 | CC0 1.0 Universal according to the publisher listing; staged animation candidate | Standard archive and extracted GLB assets are preserved under `assets/animation_candidates/quaternius_library_2/`; not connected to the live Character A animation tree |
| Quaternius Universal Animation Library Standard (2026 refresh) | [Official itch.io pack page](https://quaternius.itch.io/universal-animation-library); downloaded 2026-08-02 | CC0 1.0 Universal according to the publisher listing; isolated comparison candidate | Source archive and Godot GLB exports are preserved under `assets/animation_candidates/quaternius_library_3/`; the browser exposes the in-place export as `Library 3 — Universal (2026)` |
| RPG Character Animations Pack — Free Sample | [Godot Asset Store listing](https://store.godotengine.org/asset/explosive-llc/rpg-character-animations-pack-free/); downloaded 2026-08-01 | MIT according to the publisher listing; staged comparison candidate | Source archive and extracted `Unarmed.glb`/`Unarmed_RM.glb` are preserved under `assets/animation_candidates/godot_rpg_animation_free/`; not connected to the live Character A animation tree |
| Mixamo rifle animation downloads | [Mixamo](https://www.mixamo.com/); downloaded by project owner 2026-08-02 | Adobe Mixamo Additional Terms; staged comparison candidate | Owner-provided FBX files are preserved under `assets/animation_candidates/mixamo_rifle/`; isolated preview only, not connected to the live Character A animation tree |
| Mixamo skinned X Bot animation set | [Mixamo](https://www.mixamo.com/); downloaded by project owner 2026-08-02 | Adobe Mixamo Additional Terms; staged comparison candidate | `X Bot.fbx` and same-skeleton FBX clips are preserved under `assets/animation_candidates/mixamo_rifle_with_skin/`; isolated native-skeleton preview only |
| GaspFix pistol/rifle locomotion source pack | Owner-provided extracted folder from `Downloads\gaspfix`, received 2026-08-02 | License and original publisher still unverified; not connected to live gameplay | Preserved under `assets/animation_candidates/gaspfix/`; isolated browser at `tools/gaspfix_animation_browser.tscn` uses the pack's native UEFN mannequin |

An attempted retarget onto the Project Gunner Hero A mesh was rejected for visual quality and removed. The original Hero A runtime asset was never replaced. The retained comparison uses only the Quaternius standard mannequin and its own animation library.
