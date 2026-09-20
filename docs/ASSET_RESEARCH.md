# Motion and weapon asset research

Inspected 2026-09-19 for the requested movement/combat sandbox. Asset presence and creator previews are evidence of available source material, not proof of successful import, retargeting, or gameplay integration. Character appearance is outside this pass.

## Available locally

| Need | Exact source | Finding |
|---|---|---|
| Rifle and pistol models | `/Users/Shared/Epic Games/UE_5.8/Templates/TemplateResources/Standard/Weapons/Content/Rifle` and `/Pistol` | Rifle: 8 packages, 4.64 MB; pistol: 9 packages, 4.43 MB. Both include static and skeletal meshes, skeleton/physics, materials/textures. Rifle `M_Weapon` is also a pistol material dependency. |
| Armed locomotion and handling | `Content/Characters/Mannequins/Anims/Rifle` and `/Pistol` | Eight-direction walk/jog, ADS idle and aim offsets, equip, fire, dry fire, reload, and airborne candidates. Already copied from the installed template. |
| Template armed graph references | `/Users/Shared/Epic Games/UE_5.8/Templates/TP_FirstPerson/Content/Variant_Shooter/Anims/ABP_TP_Rifle.uasset` and `ABP_TP_Pistol.uasset` | Useful reference graphs; do not assume their template gameplay dependencies fit Gunner. |
| Immediate melee candidates | `Content/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.uasset`, `MM_Attack_02`, `MM_Attack_03`, `MM_ChargedAttack` | Existing hand-to-hand candidates; no knife-specific clip established. |
| Crouch/cover/vault/sprint in installed templates | Entire installed `Templates` filename inventory | No dedicated clips with these names found. Sprint input assets are not sprint animations. |

The installed template assets are Epic **Examples**, not CC0. The [Unreal EULA](https://www.unrealengine.com/eula/unreal), sections 1 and 5(b), distinguishes installed Templates/Samples from marketplace content and permits source/object distribution of Examples. Keep the existing Epic provenance and notices.

## Acquired official CC0 sources

Only free Standard downloads were acquired. No payment, subscription, marketplace entitlement, or new agreement acceptance was performed. The selected UAL1 in-place GLB, included `License.txt`/`README.txt`, Unreal setup image and animation inventory are retained under `ArtSource/Quaternius` (about 7.77 MB). Both included licenses explicitly identify CC0 1.0 and Quaternius. Downloaded files were inventoried directly by reading their glTF JSON animation names; no model scripts were run.

Original archives, unused Unity/root-motion variants and the complete UAL2 research download are preserved locally under **`Saved/AssetResearch/Quaternius`**, excluded from Git. `ArtSource/Quaternius/source_checksums.json` records retained source hashes; `Saved/AssetResearch/Quaternius/download_checksums.json` preserves the original acquisition inventory. The saved research archive is not required to open or play the committed prototype.

### Universal Animation Library Standard v3

[Official creator listing](https://quaternius.itch.io/universal-animation-library). Archive dated June 16, 2026; creator changelog names v3. The listing explains the switch to Unreal GLB exports to correct earlier scale/root issues and provides in-place/root-motion variants. The free archive contains **43 animations**, not the full advertised 120+ collection.

- Local research archive: `Saved/AssetResearch/Quaternius/UAL_Standard_v3/Universal Animation Library[Standard].zip`
- Archive SHA-256: `cc73fc4e495b82958207316596317a3f40b9fa38065bde1027937452da537724`
- In-place source: `ArtSource/Quaternius/UAL_Standard_v3/source/Universal Animation Library[Standard]/Unreal-Godot/UAL1_Standard.glb`
- Source SHA-256: `69591853d817488edaa8fd9bf8fc1d821eaeaf789f8627b3cd23b41c4ed67997`
- Unused root-motion companion: `Saved/AssetResearch/Quaternius/UAL_Standard_v3/source/Universal Animation Library[Standard]/Unreal-Godot/UAL1_Standard_RM.glb`.
- Complete clip/joint inventory: `ArtSource/Quaternius/UAL_Standard_v3/animation_inventory.json`.

Relevant actual clips: `Crouch_Idle_Loop`, `Crouch_Fwd_Loop`, `Sprint_Loop`, `Roll`, `Punch_Cross`, `Punch_Jab`, `Sword_Attack`, `Sword_Idle`, pistol aim up/neutral/down, pistol idle/fire/reload, and jump phases. **No backward/lateral crouch, wall-cover, or vault clips are in Standard.** Do not use the forward crouch gait to represent strafing/backpedaling while the body continues facing forward.

The current graph preserves the imported crouch torso outside ADS by fading out the upright armed layer. The selected Epic reload/equip clips are standing source motions. Crouched reload now uses project-authored arm-only composition over genuine crouch, with live acceptance in TEST_MATRIX; it is not a newly acquired source clip. The motion-polish graph also composes existing equip/dry-fire arms over genuine crouch with separate readiness gates. Dedicated crouched handling sources remain a separate acquisition gap from directional crouch locomotion.

The source rig uses `root`, `pelvis`, `spine_01..03`, paired clavicles/arms/hands and legs, plus `Head`. Similar bone names do not prove Manny compatibility: source rest pose, proportions and extra Manny spine/twist bones still need an IK Retargeter and real pose checks.

### Universal Animation Library 2 Standard v2.1

[Official creator listing](https://quaternius.itch.io/universal-animation-library-2). Creator changelog identifies v2.1; free archive contains **43 animations**. Original archive also includes a separate female mannequin; no character appearance replacement is needed.

- Local research archive: `Saved/AssetResearch/Quaternius/UAL2_Standard_v2_1/Universal Animation Library 2[Standard].zip`
- Archive SHA-256: `4008ea208a604773a2b2177d965f0f5d3195498b5bf838c3f5785d68e95f2a68`
- Local research source: `Saved/AssetResearch/Quaternius/UAL2_Standard_v2_1/source/Universal Animation Library 2[Standard]/Unreal-Godot/UAL2_Standard.glb`
- Root-motion companion: `UAL2_Standard_RM.glb` in that directory.
- Committed clip/joint research inventory: `docs/evidence/motion/ual2_research_inventory.json`; original also retained under the local research directory.

Relevant actual clips: `ClimbUp_1m`, `Slide_Start/Loop/Exit`, `Melee_Hook`/recovery, and several sword combo attacks/recoveries. `ClimbUp_1m` is a climb onto a ledge candidate, not proof of a through-vault animation. Standard has no directional crouch, dedicated cover, or knife-named clips.

## Remaining coverage and acquisition choices

The creator's [live animation viewer](https://quaternius.com/animviewer.html) was inspected through its visible search UI. Full UAL1 lists `Crouch_Bwd`, `Crouch_Bwd_L/R`, `Crouch_Fwd_L/R`, `Crouch_Left/Right`, and `Crouch_Enter/Exit` beyond Standard. The viewer labels these rows **Source**; the UAL1 Pro preview says it includes all 120+ animations. Pro is listed at **$9.99**, Source with Blender files at **$14.99**. The actual paid archive has not been acquired, so its filenames/contents are not verified. Both versions are advertised under the same CC0 pack license. This is the smallest identified direct-creator route to an entire directional crouch set, subject to paid acquisition and retarget acceptance.

Full UAL2's viewer lists `SafetyVault_RM` and `ClimbUp_2m_RM` under Source; Source is **$14.99**. Searching both full libraries for knife produced no entries; cover produced recovery clips rather than dedicated wall-cover poses. Buying these packs alone would not establish a complete cover-shooter animation library.

Two free marketplace candidates were verified at their official pages:

- [Epic Animation Starter Pack](https://www.fab.com/listings/98ff449d-79db-4f54-9303-75486c4fb9d9): 62 classic mannequin animations, free, UE-only. It needs a classic-to-Manny retarget proof. Fab in the in-app browser was signed out and displayed Add to My Library. No acquisition completed. Its actual current archive/directional crouch coverage was not inspected.
- [Advanced Locomotion System V4](https://www.fab.com/listings/ef9651a4-fb55-4866-a2d9-1b38b028f9c7): free; creator describes locomotion layering, foot IK and mantling. This is an animation/reference candidate, not authorization to replace Gunner's C++ architecture. Its current downloaded contents, entitlement and Mac compatibility were not verified.

Marketplace assets have a different distribution boundary from installed Examples. The [Epic Content License](https://www.unrealengine.com/eula/content), sections 2–4, allows development use and limits sharing source assets. If used locally, exclude the downloaded packages and retargeted derivatives from this **public** repository, preserve a local backup, and document acquisition/import steps for each developer. Do not infer a content license from a community fork's MIT code license. No unclear-source mirrors, GaspFix, or repackaged Mixamo files were used.

For a later knife mesh, [Quaternius Survival Pack](https://quaternius.com/packs/survival.html) and [Kenney Survival Kit](https://kenney.nl/assets/survival-kit) are direct-creator CC0 prop candidates; their actual knife file content has not been acquired/verified. This does not close the knife-animation gap. Existing genuine punch animations can support the user's melee alternative while knife handling remains unclaimed.

## Integration order

1. Reuse installed rifle/pistol models and armed clips, inspect sockets and both hands from the shoulder camera.
2. Import the acquired UAL1 in-place source into a separate project source-content folder; author a Manny IK retargeter and prove real crouch/sprint/roll or melee clips before enabling corresponding actions.
3. Keep directional ADS crouch and cover movement gated until their actual directional poses exist. Prefer a genuine forward crouch with travel-facing orientation over pretending a forward cycle is a strafe.
4. Integrate only acquired and validated motions. Cover entry/exit, wall lean/peek, corner traversal and true vault remain separate animation coverage gates. Do not claim the absence of a license/purchase barrier means an animation is production ready.
