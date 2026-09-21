# Gears of War 2 movement reference

Research date: 2026-09-20. Target: the original Xbox 360 **Gears of War 2**, with Horde as the desired combat rhythm. This is an action inventory and implementation reference, not a claim that Gunner reproduces the original game's animation assets, timings, networking or complete gameplay. Gunner retains its original Warden/Veyra identity.

## Evidence and confidence

- **High:** an action is explicitly described by the original manual or the publisher-hosted Gears 2 guide below. This does not establish animation durations, numerical movement speeds, collision dimensions or title-update-specific behavior.
- **Unverified:** exact timing, an inferred animation variant, or a technique supported only by another game or unspecific community discussion. Do not turn it into a Gears 2 parity claim.
- **Project proposal:** our own implementation/acceptance requirement. It is not a recovered fact about Gears 2.

The [original manual mirror](https://nuangel.net/pcdownloads/xboxmanuals/GoW2_MNL_EN-US.pdf) identifies itself as CoGFM2, part X14-94452-01. Use its printed page numbers. The similarly named first-game manual is not evidence for the sequel. The mirrored document is Microsoft/Epic primary material; the host is an archive, not the rights holder. Prima's pages are the guide publisher's own Gears 2 instructions. No original game binaries, animation files or extracted content were inspected or acquired.

## Documented core inputs

All rows below have **high action-level confidence**. Xbox notation: LS/RS are left/right sticks; LT/RT are triggers; RB is right bumper. Source: [Gears 2 manual, pp. 7–22](https://nuangel.net/pcdownloads/xboxmanuals/GoW2_MNL_EN-US.pdf).

| Action | Input / rule |
|---|---|
| Move / look | LS / RS |
| Roadie run | Hold A + direction; firing unavailable |
| Fast cover travel | Hold A while moving along cover |
| Attach | Toward cover + A |
| Detach | Move away |
| High-cover crouch | Click LS |
| Cover slip | Direction + A at corner |
| SWAT transfer | Toward adjacent cover + A; hold A interrupts |
| Mantle | Attached low cover, toward obstacle + A |
| Roll | Direction + A; available from cover |
| Aim | Hold LT; RS adjusts |
| Zoom | RS click while aiming; selected weapons |
| Hip / blind fire | RT outside / inside cover |
| Reload | RB; timed second press changes outcome |
| Melee | B; equipped weapon determines attack |
| Downed crawl | LS; repeated A accelerates |
| Revive | X near teammate |
| Heavy equipment | Heavy weapons/shield reduce mobility; Boomshot does not |
| Weapon selection | D-pad |
| Other interactions | Pickup, turret, ladder, shield, executions, grenade throwing/tagging |

## Cover behavior and sliding

**High confidence**, from [Prima's Gears 2 cover chapter](https://primagames.com/eguides/gears-war-2-eguide/welcome-back/cover):

| Behavior | Established distinction |
|---|---|
| Cover slide | Tap A near cover; running into cover while holding A automatically attaches |
| Slide direction | Forward reach is longest; side reach approximately half |
| Wallbouncing | Repeated slide, detach, slide to another nearby surface |
| Running exit | Holding A can link a cover slip into running |
| SWAT cancellation | Can redirect into a run in any direction |
| High cover | Standing/crouched edge fire; cannot shoot over its top |
| Low cover | Forces crouch; blind fire over top or raised aiming |
| Exposure | Movement, reload and firing can expose body parts; cover is not invulnerability |
| Stopping power | Incoming hits temporarily reduce movement speed |

**Unverified in this research:** exact pre-contact slide-cancel timing, accepted angles, distance in centimeters, acceleration, recovery windows, continuous rounded-corner traversal and title-update differences. The guide documents wallbouncing; it is not merely a later community invention. It does not establish every modern wallbounce technique.

The distinction matters for this implementation: a controlled movement into cover is the relevant slide behavior. A free-standing knee slide, universal parkour jump, prone stance or modern traversal chain is not established by these Gears 2 sources. Keeping Gunner's already implemented free jump/crouch is a project choice, not a parity claim.

## Handling and equipment dependencies

**High confidence**, from [Prima's Gears 2 controls chapter](https://primagames.com/eguides/gears-war-2-eguide/welcome-back/controls):

- An alternate layout separates cover from run/evade. Matching the action semantics does not require copying the default one-button layout.
- Reloads have stance/movement-dependent presentation. The guide describes timing a reload during a combat roll.
- An empty-trigger attempt can initiate automatic reload. Gunner's current dry-fire-first behavior differs.
- Active reload has success, perfect and failure outcomes; timing and benefits vary by weapon. Repeated perfect reloads narrow that opportunity until a miss or weapon swap.

**High confidence**, from [Prima's Gears 2 additions chapter](https://primagames.com/eguides/gears-war-2-eguide/welcome-back/whats-new): carrying a downed enemy restricts the player to a pistol and reduced-speed movement without running. A portable shield can be carried while running or planted as cover. These are equipment/life-state interactions, not ordinary locomotion poses.

For Gunner, health, downed teammates/enemies, shield items, usable turrets, grenades and their authority rules must exist before those branches can be accepted. An isolated crawl, revive or execution animation would not constitute the corresponding system. Horde AI, waves and those dependent gameplay systems remain outside the current movement pass unless separately expanded by the user.

## Version boundaries

- The [Gears 3 advanced movement guide](https://primagames.com/eguides/gears-war-3-eguide/multiplayer/advanced-movement) explicitly teaches opposite-direction cover-slide cancellation and roll-to-run switching. It is evidence about Gears 3, not sufficient evidence for Gears 2 timing. Test a Gears 2 build before making frame-accurate claims.
- The Coalition identifies close-cover combat as introduced in Gears 4 in its [Bootcamp description](https://www.gearsofwar.com/en-us/news/bootcamp/). The [Gears 4 control guide](https://primagames.com/eguides/gears-of-war-4-eguide/basic-training/controls) details vault kicks, enemy yanks and knife finishers. Those combinations should not be presented as the original Gears 2 move set.
- Community names such as reaction shots, wrap shots, air-bouncing or hyper-bouncing do not establish a separate animation requirement by themselves. No version-specific primary evidence for their precise Gears 2 implementation was established here.
- No numeric camera settings, sensitivity curves, animation catalogue or movement constants were recovered. Any Gunner values are authored tuning and require playtesting.

## Gunner implementation and animation acceptance map

The following is **project design**, derived from the desired action coverage and the current [animation pipeline](../ANIMATION_PIPELINE.md), not a list of extracted Gears assets. Each row needs both behavior and visible animation; file presence alone does not complete it.

| Workstream | Required original/licensed coverage | Acceptance requirement |
|---|---|---|
| Ground locomotion | Rifle/pistol idle, directional travel, starts/stops, turns | Responsive input, consistent contact, no speed spikes through transitions |
| Combat sprint | Compact armed run, acceleration/braking transitions, steering presentation | Predictable steering, smooth camera, combat guard and reliable release |
| Cover approach | High/low entry and braking, alternate contact sides | Swept approach, valid contact, interruption, no capsule teleport or foot sliding |
| Cover stance | High standing/crouch, low crouch, left/right stationary and lateral travel | Stable wall contact, protected head and meaningful direction changes |
| Fast cover travel | Faster left/right high/low travel | Motion matches travel rate; no forward-only crouch clip mislabeled as strafing |
| Corner escape | Left/right departure into open travel/run | Checked floor and route, no stuck plane constraint |
| Nearby transfer | Left/right transition between valid surfaces | Destination reservation/validation, obstacle rejection and safe cancellation |
| Low obstacle crossing | Genuine through-vault/mantle motion | Entire capsule route and landing clear; hands/feet match obstacle; safe interruption |
| Evade | Roll in requested direction and recovery | Direction choice, wall/gap rejection and clean return; cover exit and reload interactions explicit |
| Protected attacks | High left/right aiming/blind fire, low top/edge aiming/blind fire | Actual muzzle clearance, protected body and continuous mouse look |
| Handling | Reload/equip/fire/dry feedback across valid stance states | Weapon grips and stance preserved; no stale ammo or damage commits |
| Incapacity | Down transition, crawl, recovery, assisted interaction | Deferred until health/revival authority and partner behavior exist |
| Equipment branches | Carry, deploy, mounted handling and their transitions | Deferred until their corresponding original items/gameplay exist |

Prioritize the cover travel/transition gaps before declaring movement complete. Existing sprint, roll and crouch clips are reusable foundations, not dedicated cover clips. Keep unavailable animation-dependent actions gated. Preserve the foundation map and use an authored test course for transitions, obstructions, corners, low/high cover, slopes and interruption. Validate from the actual shoulder camera with both weapons, including repeated mixed-action sequences.

## Remaining reference capture work

A manual specifies commands but cannot settle the requested feel. For a later side-by-side comparison, record a legally owned Gears 2 build with its title-update/version identified: input-to-motion delay, approach distance, cover attachment and exit, run turning, roll recovery, camera response, both cover heights, both sides and supported handling overlaps. Until that capture exists, describe Gunner as an original interpretation of the documented actions, not a measured recreation of the original controller.
