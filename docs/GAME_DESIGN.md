# Game Design

## Identity and pillars

**Project Gunner: Rift Bastion** is a compact cooperative defense game about deliberate movement, readable cover choices, and timing environmental defenses. Its identity is solar-industrial science fiction rather than military gothic.

1. Heavy but responsive Warden movement.
2. Cover is a tactical tool, never a passive invulnerability state.
3. The arena is a weapon: pulse fences create breathing room.
4. The Veyra read as fast crystalline pack hunters.

## Core loop

Select one or two Wardens, enter the crater bastion, survive three Veyra waves, reload and reposition during breaks, activate pulse defenses when lanes collapse, and either clear the final wave or lose when every Warden is dead.

## Rules

- Solo or two-player local cooperative play.
- Crouch is a deliberate toggle with reduced speed, lower camera framing, a compact collision profile, and a standing-clearance check. It remains available while attached to cover, including lateral cover movement and firing.
- In the local movement profile, the traversal button enters nearby cover, rolls on a short open-space tap, and sprints on hold; attached hold enables faster wall travel and away input detaches. Jump has a separate action. Corners, transfers and a cover vault remain intended movement coverage, gated until their animation and collision routes are proven. The portable baseline retains cover/detach/jump fallback.
- Three waves with rising enemy count, resilience, speed, and pressure.
- An enemy counts as remaining from reservation through death, so the HUD never understates the threat.
- A Warden at zero health becomes incapacitated before death. A living teammate can revive nearby; a solo incapacitation expires into defeat.
- The team wins when the final wave has no reserved or living enemies.
- The team loses when all configured players are dead.
- Match results always offer restart and return to menu.

## Characters

- **Character A — Micah Vale / Field Warden:** charcoal and muted-olive layered field gear, selective gunmetal protection, asymmetrical chestnut hair, and a teal upper-back relay vane.
- **Character B — Rook Nadir / Ranger:** amber armor, asymmetric shoulder fin, orange reactor core.

They currently share combat tuning so selection is expressive rather than a balance advantage.
