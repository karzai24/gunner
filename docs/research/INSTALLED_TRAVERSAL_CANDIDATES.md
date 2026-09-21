# Installed Epic traversal candidates

The installed UE 5.8.2 `MoverExamples` plugin contains seven Manny crouch clips and `VaultOver`. `Tools/inspect_installed_traversal.py` inspected these in a separate commandlet on 2026-09-20 through a temporary content mount. No Mover/ChaosMover gameplay plugin was enabled. The source and project skeletons are distinct assets but have identical names, order, parents and local reference transforms within the recorded tolerances. The local report is `Saved/installed_traversal_inventory.json`.

These are installed Epic plugin assets, not Quaternius CC0 motion or automatically redistributable installed-template Examples. Source packages remain untouched in the engine. Project derivatives and their composed graph/settings remain under the ignored `Content/Gunner/LicensedLocal/` directory. Commit the reproducible scripts, not these packages. Final distribution/provenance decisions are separate from this local evaluation.

## Observed source poses

Values below are sampled bone positions relative to the animation root, not a guarantee of world-space cover protection. The inspector records 61 times per clip, both foot-relative and root-relative heights, full poses, package dependencies and source SHA-256 values.

| Source clip | Duration | Head height above root |
|---|---:|---:|
| Crouch idle | 9.433 s | 89.0–91.1 cm |
| Crouch entry | 0.733 s | 90.2–159.7 cm |
| Crouch exit | 0.867 s | 90.2–159.7 cm |
| Crouch forward | 1.800 s | 114.1–120.0 cm |
| Crouch backward | 1.733 s | 115.8–122.0 cm |
| Crouch left | 1.733 s | 119.6–124.0 cm |
| Crouch right | 1.333 s | 114.8–121.3 cm |
| VaultOver | 2.033 s | 60.9–169.7 cm |

Moving crouch sources are too tall to replace the existing protected low-cover gait behind 115 cm barriers. The composition therefore keeps the original Quaternius `BS_Crouch` branch for low cover. The new directional branch is for free crouch and high cover. VaultOver's recorded root travels approximately 390.1 cm forward and reaches 123.3 cm above its initial root; this is a source trajectory, not an accepted collision path for Gunner's barricades.

## Local installer and asset gate

`Tools/install_installed_traversal.py` requires the matching inspection hashes and canonical-skeleton proof. The native `DuplicateCompatibleSequence` helper independently checks every reference bone, creates a new package only under `/Game/Gunner/LicensedLocal`, rebinds an independent editable copy, clears old retarget/preview references and verifies its raw data GUID/duration. It never converts or overwrites the engine source.

The installer creates eight local copies. Five supply `BS_DirectionalCrouch`: idle at zero and forward/backward/left/right at signed crouch speed. Their source displacement is 300 cm/s; moving copies use `CrouchSpeed / 300` playback scaling and explicit root locking. The graph's native rate input handles faster supported cover travel. VaultOver remains unassigned. Stationary free/high-cover crouch entry/exit is selected at 2.5x playback and passed focused rendered stance/carry checks. These two clips bring the active installed set to seven crouch clips. Movement, aim and actions cancel presentation; protected low cover clears it immediately.

The local `ABP_WardenTraversal` preserves blind fire and protective handling while selecting the separate low-cover gait. The local `DA_MotionTraversal` enables directional crouch and contextual controls without editing the original shared settings. During this intermediate authoring stage, the existing `BP_WardenMotion` assignment changes outside the ignored directory. Final `enable_local_movement.py` restores that package to its verified committed baseline and selects the final graph through ignored config instead. Its previous package and sidecars are copied into `Saved/AuthoringDrafts/before-installed-traversal-*`; `Saved/installed_traversal_authoring_report.json` records the exact backup, changed package and rollback instructions. A local Blueprint assignment referencing ignored assets must not be committed without a corresponding reproduction strategy.

`Tools/validate_installed_traversal_assets.py` is a read-only persisted-asset gate. It compares every editable source bone-frame transform and timing, checks skeleton/duration/root policy, signed Blend Space samples and playback rates, graph capabilities, absence of plugin package references, and unchanged original Quaternius packages. It does not compare curve tangents; evaluated axis bindings are covered by the rendered traversal probe. Its report is `Saved/installed_traversal_asset_validation.json`. Passing this gate does not certify live weapon-hand contact, gait quality or protection; those remain rendered native-probe and visual acceptance work in `TEST_MATRIX.md`.

The installed persisted-asset gate passed in a fresh process. An earlier local Mixamo profile and 24 traversal scenarios in each of two rendered PIE sessions passed focused gates, but a later normal-editor restart auto-reimported and corrupted the Mixamo target poses. The source-layout/import-metadata repair and fresh recovery persistence gate now pass (2,013 poses across eleven clips and fourteen package hashes). The recovered 30-scenario traversal run (612 checks), movement (186), blind-fire (68), crouched-reload (224) and polish (242) regressions now pass, including stance transitions and arm-only carry. Portable fallback also passed 184 checks; post-normal-editor persistence revalidated all 2,013 poses and fourteen package hashes with zero errors/warnings. Final acceptance and limitations are recorded; the earlier 24-scenario pass is excluded. Current status belongs in TEST_MATRIX. The intermediate character assignment must not be committed; [LOCAL_MOVEMENT_SETUP](../LOCAL_MOVEMENT_SETUP.md) describes the complete restoration/config sequence.
