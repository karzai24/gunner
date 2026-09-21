# Optional local movement profile

The local profile has been recovered after a restart-triggered auto-reimport corrupted its retargeted poses. The recovery gate and all six two-session rendered suites passed: 1,516 checks across twelve PIE sessions, zero failures. Portable fallback accounts for 184 of that total; post-normal-editor persistence revalidated all 2,013 poses and fourteen package hashes with zero errors/warnings. See TEST_MATRIX for the evidence and limitations. Ordinary setup must not rerun the authoring scripts over existing content. A fresh clone uses the committed portable motion graph because Mixamo and installed engine-plugin source/derived assets are deliberately excluded from Git.

The profile adds genuine directional crouch outside low cover, contextual tap/hold traversal, a hunched rifle sprint and rifle high-wall idle/lateral poses. It preserves the existing weapon handling and protective low-cover composition. It does not enable knee slides, corners, transfers, vaults or knife attacks. See [ANIMATION_PIPELINE](ANIMATION_PIPELINE.md) for selected clips and limitations.

## Sources and preparation

Use UE 5.8.2 with its installed Mover Examples content and the project Editor build. Keep only one Unreal process open on this 16 GB development machine. Save and close any authoring session before running setup; the normal-editor Mixamo wrapper exits the editor when finished.

Download the exact eleven animation titles in [MIXAMO_SOURCE_MANIFEST](research/MIXAMO_SOURCE_MANIFEST.json) from the official signed-in [Mixamo catalog](https://www.mixamo.com/). Use X Bot, FBX Binary, 30 fps, no keyframe reduction and Without Skin for animation clips; download the source X Bot T-pose with skin. Select In Place for the run and left/right wall loops. Preserve the transition exports as supplied. Rename files to the manifest's local filenames and place them under `Saved/AssetResearch/Mixamo/`, with `manifest.json` using that metadata. Hash mismatches require inspection of the new export, not editing the expected hash simply to bypass a guard. No credentials or animation data belong in Git.

Retained Mixamo FBX copies belong under ignored `Saved/LicensedSources/Mixamo`, outside Content. Imported `.uasset` source/target packages remain under `Content/Gunner/LicensedLocal/Mixamo`. Retargeted sequences must have fresh generic empty `UAssetImportData`, assigned through native `ClearDerivedAnimationReimportSource` because the UE 5.8 Python property is read-only; keep provenance in manifests/reports instead of advertising X Bot FBX as a direct reimport source. The original Content/SourceFiles layout caused folder watching to reimport 23 assets through Interchange and corrupt target poses after restart. The affected tree was backed up and recovered with this source policy. A fresh gate passed 2,013 poses across eleven clips and fourteen package hashes; the normal-editor restart and rendered regression requirements still apply.

The installed source inspection temporarily mounts `/MoverExamples/` content and compares its skeleton with canonical Manny. It does not enable the experimental movement plugins or replace ACharacter. Source packages and editable derivatives stay local; see [ASSET_PROVENANCE](ASSET_PROVENANCE.md).

## One-time reproduction sequence

Run each stage to completion before the next. Reports are under `Saved`; failed or unexpected existing outputs deliberately stop setup. Preserve recovery folders and inspect the failure rather than deleting assets or removing guards.

| Order | Script | Purpose |
|---|---|---|
| 1 | `inspect_installed_traversal.py` | Inspect installed crouch/vault sources, reference-pose equivalence, hashes and sampled trajectories |
| 2 | `install_installed_traversal.py` | Create eight independent canonical copies, directional crouch blend/graph/settings, and recoverable temporary character assignment |
| 3 | `validate_installed_traversal_assets.py` | Fresh-process source-pose, skeleton, rate, dependency and composition gate |
| 4 | `prepare_mixamo_editor.py` | Import the manifest and retarget all eleven clips in a normal Slate editor process |
| 5 | `install_local_movement.py` | Create final local graph, wall Blend Space and settings; refuse existing outputs |
| 6 | `enable_local_movement.py` | Standard Python after Unreal closes: verify hashes, preserve backups, restore committed character package and write ignored local config |
| 7 | Normal-editor restart, then `validate_mixamo_persistence.py` and `validate_local_movement_assets.py` | Prove folder watching does not reimport/rewrite targets; compare source/target pose and package hashes, import metadata and portable Blueprint identity |
| 8 | Rendered probes and visual inspection | Exercise actual movement plus motion, blind-fire, crouched-reload and polish regressions; record results in TEST_MATRIX |

Build first:

```bash
./Tools/build_macos.sh GunnerEditor
```

For stages 1–3, 5 and 7, use a fresh commandlet with the appropriate script name (shown here for stage 1):

```bash
./Tools/open_editor_macos.sh -run=pythonscript \
  "-script=$PWD/Tools/inspect_installed_traversal.py" \
  -NullRHI -unattended -nosound -nop4 -FullStdOutLogOutput -stdout
```

Run `validate_mixamo_persistence.py` in a separate commandlet after import/retarget exits, and again after a normal-editor session. It compares all 61 recorded pose samples for source sequences and both raw/root-locked target evaluations, original FBX hashes, reference skeletons, import paths/metadata and fourteen saved rig/target package hashes. Its fresh post-recovery run passed all 2,013 pose samples and fourteen package hashes. This does not replace rendered runtime validation.

Stage 4 requires a normal editor: the UE 5.8 FBX mesh importer needs Slate and failed in a headless commandlet.

```bash
./Tools/open_editor_macos.sh -nosound \
  "-ExecCmds=py $PWD/Tools/prepare_mixamo_editor.py" -FullStdOutLogOutput -stdout
```

After stage 5 has finished and Unreal has closed:

```bash
python3 Tools/enable_local_movement.py
```

The installer preserves the original character under `Saved/AuthoringDrafts`. Enablement checks that recovery file against the committed Git blob before restoring it, then writes the following section in ignored `Saved/Config/MacEditor/Game.ini` and `Saved/Config/Mac/Game.ini`:

```ini
[Gunner.LocalMotion]
Profile=/Game/Gunner/LicensedLocal/Mixamo/Runtime/DA_CoverMovement
```

The native loader verifies the path, base class and exact skeleton before using it. It runs only for characters with motion settings, leaving the foundation pawn unaffected. A missing/incompatible profile falls back to the portable graph. Existing config or changed character packages cause the enable script to refuse replacement.

## Validation and fallback

Run the capability-required traversal probe in the rendered editor:

```bash
./Tools/open_editor_macos.sh -GunnerTraversalSmoke \
  -GunnerRequireTraversalCapabilities -nosound \
  "-ExecCmds=py $PWD/Tools/validate_traversal_editor.py" -FullStdOutLogOutput -stdout
```

The driver runs two PIE sessions. Require two completion markers with `failures=0`, `skipped=0` and the complete current scenario count, then inspect the actual screenshots. The current probe covers 30 scenarios per session, including stance transitions/carry/interruption, supported/blocked entry, rolls, directional crouched ADS, protected low cover, contextual sprint/look and rifle wall grip/gaits. Both rendered sessions passed 306 checks each: 612 passes, zero failures or skipped capabilities. The older 24-scenario pass predates auto-reimport corruption and must not be reused as current validation.

A fresh commandlet is insufficient for persistence acceptance: launch a normal editor with default folder watching, allow its startup/import work to settle, close cleanly, and rerun package/pose checks in another process before rendered probes. Reject unexpected Interchange/reimport events and changed target poses. The recovery and 30-scenario rendered gates passed, and persistence passed again after the complete final normal-editor regression sequence. Repeat that post-editor check when reproducing the setup. Script presence alone is not a pass. These checks do not establish every requested movement family or physical-device acceptance.

To exercise the portable composition without deleting any local source or config:

```bash
./Tools/open_editor_macos.sh -GunnerIgnoreLocalMotion
```

Local source and graph paths remain excluded from the public repository. Cooking/packaging the optional profile needs an explicit content-inclusion and distribution review; the soft local config does not establish a tested packaged release.
