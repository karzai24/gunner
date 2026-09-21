# Cover movement evidence

Collected from the repaired local movement profile on 2026-09-20 (America/Los_Angeles), UE 5.8.2, Mac Development. Log timestamps after midnight use UTC.

| Evidence | Local source | Result / scope |
|---|---|---|
| [build-editor.txt](build-editor.txt) | `Saved/cover-movement-final-build-editor.log` | GunnerEditor build succeeded |
| [build-game.txt](build-game.txt) | `Saved/cover-movement-final-build-game.log` | Gunner game build succeeded; installed MetalShaderConverter include-directory and staging warnings retained |
| [traversal.txt](traversal.txt) | `Saved/cover-movement-30-full.log` | Two rendered PIE sessions, each `failures=0 scenarios=30 skipped=0`, then complete teardown |
| [assets.json](assets.json) | `Saved/local_movement_asset_validation.json` | Local profile composition, capabilities, portable character identity and 14 retargeted package hashes passed |
| [installed-assets.json](installed-assets.json) | `Saved/installed_traversal_asset_validation.json` | Eight canonical copies, editable-data hashes, root/rate settings, directional samples and preserved originals passed |
| [motion.txt](motion.txt) | `Saved/cover-movement-final-motion.log` | 186 passes / two completed local-profile sessions |
| [blind-fire.txt](blind-fire.txt) | `Saved/cover-movement-final-blind-fire.log` | 68 passes / two completed sessions |
| [crouch-reload.txt](crouch-reload.txt) | `Saved/cover-movement-final-crouch-reload.log` | 224 passes / two completed sessions |
| [polish.txt](polish.txt) | `Saved/cover-movement-final-polish.log` | 242 passes / two completed sessions |
| [portable-motion.txt](portable-motion.txt) | `Saved/cover-movement-final-portable-motion.log` | 184 passes / two completed portable fallback sessions |
| [persistence.json](persistence.json), [log](persistence.txt) | Final post-editor persistence commandlet | 2,013 poses / eleven clips / fourteen unchanged package hashes; zero errors/warnings |
| [assets.txt](assets.txt) | `Saved/cover-movement-final-asset-reload.log` | Final fresh graph reload gate; zero errors/warnings |

Build logs replace machine paths with `<PROJECT>`, `<UE_ROOT>`, `<USER_HOME>`, `<XCODE>` and `<TEMP>`. The traversal excerpt retains all 663 `GUNNER` lines, PIE teardown and warning/error/fatal-level messages. One analytics request-detail line containing a URL and identity IDs is omitted. Engine motion-vector thread-safety and outstanding HTTP shutdown warnings remain visible.

The twelve traversal PNGs are unmodified, byte-identical copies from `Saved/Screenshots`, captured in the second PIE cycle. They show crouched high-cover peeking (18), turning during contextual sprint (20), rifle wall idle/left/right (22), wall dry fire (24), and stance down/up/crouched carry on open ground (26) and at high cover (27). PNGs contain only image chunks, with no embedded text/location metadata.

The JSON files contain aggregate validation metadata: asset identifiers, hashes, counts, durations, settings and Blend Space coordinates. No animation source files, editable keys or sampled pose inventories are distributed here. Persistence and combat-regression reports likewise contain metadata and measurements only. The six additional regression PNGs show rifle/pistol blind fire, rifle cover/pistol free reload, a cross strike and protected equip; they are also unchanged second-cycle gameplay captures.

These are synthetic-input rendered probes and selected stills, not physical mouse/controller playtesting or proof of complete Gears-style movement. Stance checks measure motion range and completed stance/carry; the numerical support-hand grip assertion covers the crouched endpoint, not every frame of entry/exit. Knee slides, corners, transfers, vault gameplay, pistol-specific wall poses and knife handling remain outside this evidence. The accepted total is 1,516 checks across twelve completed sessions, with zero failures. Final scope and limits are recorded in [TEST_MATRIX](../../TEST_MATRIX.md).
