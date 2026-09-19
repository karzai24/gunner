# Rift Bastion Vertical Slice — 2026-08-01

## Playable

- Complete launch → mode selection → character selection → match → victory/defeat → restart/menu loop.
- Solo and local two-player split-screen horde defense.
- Micah Vale custom rigged Character A and Rook Nadir procedural Character B placeholder, with distinct silhouettes, palettes, and upper-back identifiers.
- Third-person movement, sprint, clearance-safe crouch in free locomotion or attached cover, contextual jump, shoulder camera, aim, hitscan shooting, recoil, ammo, reload, health, incapacitation, teammate revive, death, cover, and low-cover vault.
- Character A locomotion tuning: normal movement now selects the run clip at gameplay speed, sprint waits for acceleration, walk/run switching has hysteresis, and locomotion blends use longer transitions to reduce snapping and foot-slide perception.
- Three escalating Veyra waves, intermissions, bounded spawning, navigation/attacks, and authoritative results.
- Two pulse-fence environmental defenses with active field, slow, damage, and cooldown feedback.
- Original low-poly arena, Blender-authored Character A, procedural Character B/enemy/weapon, UI, VFX, and synthesized audio cues.
- Settings for mouse/stick aim, aim multiplier, inversion, camera shake, and reduced flashes.
- Pause, credits, export presets, visual captures, smoke tests, and performance probe.

## Known limitations

See `docs/TEST_MATRIX.md`. LAN gameplay, physical-controller hardware QA, runtime remapping, melee, weapon switching, Character B production art, and facial animation remain outside this local vertical slice.
