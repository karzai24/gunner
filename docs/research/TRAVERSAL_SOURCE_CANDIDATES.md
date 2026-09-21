# UAL2 slide source preparation

Prepared 2026-09-20. **Scripts have been syntax-checked, not executed in Unreal as part of this preparation.** No imported or retargeted output is accepted by this document. The runtime and live pose validation gate remains in [ANIMATION_PIPELINE](../ANIMATION_PIPELINE.md).

The existing Quaternius Universal Animation Library 2 Standard v2.1 research download contains the genuine `Slide_Start`, `Slide_Loop` and `Slide_Exit` animations. These are generic slides. They are not acquired Gears animations or proof of suitable wall contact, cover entry, protective stance or weapon-hand alignment. They may be rejected after rendered inspection. In particular, a slide candidate must not be called a through-vault or dedicated cover slam.

The current task's separate source inspection identified a seated, feet-first slide (approximately 5 cm pelvis and 48 cm head height), so these candidates are unsuitable as the requested knee/cover slide. Keep the scripts available for reproducible source research; do not activate these clips as cover-entry coverage.

## Source and preservation

Existing source directory:

`Saved/AssetResearch/Quaternius/UAL2_Standard_v2_1/source/Universal Animation Library 2[Standard]`

`Unreal-Godot/UAL2_Standard.glb` is the in-place edition. Its verified SHA-256 is `8cee20ab1bc55130092447e810e26df22dd2803eccc54f52137a7d54d7ab88a8`. The original `License.txt` identifies Quaternius and CC0 1.0; the README distinguishes the separate `_RM` edition. This workflow does not use `_RM` or the Unity FBX. Prior acquisition and archive hashes remain in [ASSET_PROVENANCE](../ASSET_PROVENANCE.md).

`Tools/import_traversal_source.py` first validates the source hash, GLB header, 43 animation names, one skin and candidate durations. On execution it preserves the unmodified GLB, license, README and Unreal setup image under the corresponding `ArtSource/Quaternius/UAL2_Standard_v2_1/source/Universal Animation Library 2[Standard]` directory. Identical preserved files may be reused; differing files are never replaced. Existing UAL2 package directories cause a refusal before source copying/import. Failed partial imports remain available for inspection and are not silently deleted or overwritten on retry.

The full GLB source library imports separately under:

`/Game/Gunner/Animation/Source/Quaternius/UAL2_Standard`

The expected mesh is `SkeletalMeshes/UAL2_Standard`. Expected source sequence names are `SkeletalMeshes/UAL2_Standard<clip>`. Importing all 43 source sequences does not activate any action. The report `Saved/traversal_import_inventory.json` records source checksums, source/imported joint agreement, mesh/skeleton identity, packages, durations and source root settings.

## Bounded retarget outputs

`Tools/retarget_traversal.py` selects only these three sequences:

| Source suffix | GLB duration | Expected output under `/Game/Gunner/Animation/Retargeted/Manny` |
|---|---:|---|
| `Slide_Start` | 0.833333 seconds | `A_UAL2_Slide_Start` |
| `Slide_Loop` | 2.0 seconds | `A_UAL2_Slide_Loop` |
| `Slide_Exit` | 0.5 seconds | `A_UAL2_Slide_Exit` |

Durations were read from the existing GLB input-accessor bounds without launching Unreal. Import and retarget both check them within 0.001 seconds.

Three **new** assets under `/Game/Gunner/Animation/Rigs` avoid assuming that the UAL1 and UAL2 skeleton objects are interchangeable:

- `IK_QuaterniusUAL2`, assigned to the actual imported UAL2 mesh/skeleton.
- `IK_MannyTraversal`, assigned to `/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple`.
- `RTG_QuaterniusUAL2_Manny`, using the above rigs, actual preview meshes, validated humanoid chains, target alignment and foot grounding.

Existing UAL1/Manny rigs and retargeters are left intact. Every expected output is checked before creating assets. Referenced-asset retargeting and overwriting are disabled. The output must contain exactly three clips, target the actual Manny skeleton, and preserve duration. Source sequences and settings are inspected again to detect changes.

Target clips explicitly set animation root motion off, force root lock on and root-lock mode `REF_POSE`. CharacterMovement remains responsible for any later displacement. The script evaluates root-lock-respecting poses, checks finite root/pelvis/head/hand/foot transforms and rejects sampled root translation drift above 0.01 cm. This checks extracted pose data, not rendered deformation or foot contact. Samples, chain mappings, skeleton identities, settings and measured root drift are recorded in `Saved/traversal_retarget_report.json`.

## Serial execution and remaining validation

Run the import before retargeting, using the installed Unreal 5.8 editor commandlet with `-run=pythonscript`, an absolute `-script` path and `-NullRHI`. Only one Unreal process should run at a time on this machine. The scripts neither build montages nor assign the character graph nor enable gameplay.

After both scripts succeed, inspect all three clips on actual Manny in rendered gameplay with rifle and pistol. Validate stance height, hand contact, start/loop/exit continuity, foot contact, obstruction handling, transitions and interruption before choosing any candidate for runtime use. A successful import/retarget is not movement-system completion.
