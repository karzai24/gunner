# Level Design — Rift Bastion

## Metrics

- Player capsule: 0.72 m diameter, 1.8 m tall.
- Crouched capsule: 0.72 m diameter, 1.24 m tall; standing requires 0.56 m of clear added headroom.
- Low cover: 1.15 m high; high cover: 1.8 m.
- Cover snap distance: 1.35 m; attached offset: 0.52 m.
- Primary lanes: at least 4.5 m wide.
- Camera operating radius: 3.2 m with swept obstruction handling.
- Arena footprint: 44 × 32 m.

## Arena grammar

The central reactor ring is the orientation landmark. North and south breach gates feed diagonal pressure into three circulation routes: the open center, west cover weave, and east defense lane. Cover islands are deliberately offset so no position protects every approach. Two pulse-fence consoles sit near lane pivots and reward moving out from the safest central cover.

Spawn gates begin outside direct player view and provide several seconds of traversal. The outer wall and floor collision prevent out-of-bounds progression failures. Low covers expose enemies that reach flanks; tall pylons break long sightlines without becoming dead ends.

## Defense behavior

An active pulse fence creates an 8-second slowing/damage field. It has a visible cooldown and cannot be spammed. Enemies continue to have a valid route around the field; the tool buys time rather than permanently blocking a wave.
