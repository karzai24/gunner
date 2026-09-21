# Level Design — Rift Bastion

## Metrics

- Player capsule: 0.72 m diameter, 1.8 m tall.
- Crouched capsule: 0.72 m diameter, 1.24 m tall; standing requires 0.56 m of clear added headroom.
- Low cover: 1.15 m high; high cover: 1.8 m.
- Cover snap distance: 1.35 m; attached offset: 0.52 m.
- Primary lanes: at least 4.5 m wide.
- Camera operating radius: 3.2 m with swept obstruction handling.
- Arena footprint: 44 × 32 m.

## Current movement range

The current sandbox is `Content/Gunner/Motion/Maps/L_MotionRange`, a separate copy of the 44 × 32 m foundation room. It contains 1.15 m and 1.8 m cover fixtures, open movement lanes and three resetting weapon targets. The final arena below remains a design target. The foundation map is preserved.

High-cover edge exposure adds a checked 0.70 m lateral step from the protected anchor; the 0.52 m capsule offset remains unchanged. The optional rifle wall pose moves the rendered skeletal root 0.25 m toward the wall to fit its authored lean; it does not move the collision anchor. Roll travel is at most 3.50 m and retains the standing capsule, so a crouched start cannot pass under low openings.

Cover approaches use swept movement at 3.00 m/s normally or 4.50 m/s from a fast approach, with floor samples at intervals no greater than 0.70 m. Attached travel also checks braking distance before a wall or floor ends. The local movement profile uses 3.00 m/s normal travel, 5.00 m/s sprint and 1.40 m/s crouch; standing wall travel is 1.50 m/s or 2.20 m/s while the context button is held. These are original tuning choices for the available motion, not measured franchise values. Room, capsule, cover and lane dimensions are unchanged.

The inspected installed vault travels approximately 3.90 m and does not fit a full launch from the existing 0.52 m cover anchor. It remains disabled; do not change the fixtures or teleport the player to conceal that mismatch.

## Arena grammar

The central reactor ring is the orientation landmark. North and south breach gates feed diagonal pressure into three circulation routes: the open center, west cover weave, and east defense lane. Cover islands are deliberately offset so no position protects every approach. Two pulse-fence consoles sit near lane pivots and reward moving out from the safest central cover.

Spawn gates begin outside direct player view and provide several seconds of traversal. The outer wall and floor collision prevent out-of-bounds progression failures. Low covers expose enemies that reach flanks; tall pylons break long sightlines without becoming dead ends.

## Defense behavior

An active pulse fence creates an 8-second slowing/damage field. It has a visible cooldown and cannot be spammed. Enemies continue to have a valid route around the field; the tool buys time rather than permanently blocking a wave.
