# Test Matrix

Validated on 2026-08-01 with Godot 4.7 stable, Jolt Physics, GL Compatibility, Windows, and an NVIDIA GeForce GTX 1650.

| Flow | Automated evidence | Visual/manual evidence | Status |
|---|---|---|---|
| Parse/import and boot to menu | Clean editor/import and startup runs | `tests/artifacts/main_menu.png`, 1600×900 | Pass |
| Solo selection and match spawn | Solo travel, one pawn, one viewport | GL-rendered solo capture during first visual pass | Pass |
| Duo character selection | Both stable character IDs checked | `tests/artifacts/character_select.png` | Pass |
| Hero A GLB/rig/animation import | Six meshes, one skeleton, 28 required bones/sockets, 12 clips, six surfaces, active `COLOR_0` layers, and walk/run threshold-hysteresis checks from `tools/hero_a_validation.tscn` | Gameplay locomotion captures: `tests/artifacts/hero_a_walk.png`, `tests/artifacts/hero_a_run.png` | Pass |
| Aim/fire/ammo/reload | Input-to-weapon round consumption and reload commit | Crosshair/ammo HUD in combat captures | Pass |
| Crouch/jump traversal | Explicit state, capsule/camera lowering, blocked standing, contextual jump, crouched cover movement, firing, and exit-stance checks | `tests/artifacts/crouched_cover.png` | Pass |
| Health/incapacitation/revive/death | Damage idempotency and teammate revive checks | Damage/result presentation inspected | Pass |
| Cover enter/move/exit/vault | Guarded cover state and attach/exit checks; vault destination guard parses | Low covers visible and reachable | Pass; vault input remains manual-only |
| Enemy spawn/pursue/attack/death | Active pooled Veyra spawn and lethal damage check | Active Veyra visible in combat capture | Pass |
| Defense activation/slow/cooldown | Activation, multiplier, and repeat rejection checks | Cyan active-field treatment inspected | Pass |
| Three waves and victory | Accelerated authoritative wave-clear smoke | Victory overlay uses same result component as captured defeat | Pass |
| All players dead and defeat | Forced authoritative defeat smoke | `tests/artifacts/match_failed.png` | Pass |
| Pause/resume | State and overlay visibility checks | Focusable pause UI constructed from production theme | Pass |
| Restart and return to menu | Repeated travel/unload smoke | Result buttons inspected | Pass |
| Two-player split-screen | Two pawn inputs, viewports, cameras, and HUD contracts | `tests/artifacts/split_screen.png` | Pass |
| Development export pack | Godot `--export-pack` using Windows Development preset | `tests/artifacts/rift_bastion_dev.pck` | Pass |
| LAN host/client gameplay | Transport boundary only | None | Not implemented |

The automated smoke suite passes 60 checks without errors, warnings, orphan nodes, or leaked ObjectDB instances. Controller semantic input is simulated in the harness; no physical controller was available for hardware-level dead-zone or vibration verification.

## Performance evidence

`tests/artifacts/performance.txt` records 300 frames in the heaviest current scenario: two local viewports, the 28,392-triangle Micah plus the Character B placeholder, seven active wave-3 Veyra, and two active defenses. The final run held 59.80 average FPS, averaged 18.929 ms paced frame interval, peaked at 31.834 ms, averaged 2.538 ms physics, peaked at 3.234 ms physics, and recorded no frame intervals above 33 ms. It peaked at 761 draw calls, 913 rendered objects, and 81.34 MiB static memory.

## Known limitations

- LAN ENet host/join transport methods exist, but snapshot replication, prediction/reconciliation, lag compensation, lobby UI, and adverse-network tests are not implemented or exposed.
- The slice has one weapon, one enemy archetype, one arena, and one mode by design.
- Character A uses the documented reusable humanoid skeleton and an imported 12-clip `AnimationTree`; Character B still uses procedural part animation and needs a future production-art pass.
- Runtime input remapping, melee, and weapon switching are not included in this scoped slice.
- Windows export presets and a PCK were validated; a standalone EXE requires matching Godot 4.7 export templates on the machine.
