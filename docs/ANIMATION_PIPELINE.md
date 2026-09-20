# Humanoid animation contract

## Canonical skeleton and scope

Epic UE5 Manny/Quinn-compatible `SK_Mannequin` remains the provisional gameplay skeleton. `SKM_Manny_Simple` is the current visual. Custom Micah/Rook appearance waits until movement and weapon interaction are proven. The original foundation BP_Warden and installed ABP_Unarmed remain available; BP_WardenMotion now uses the project `ABP_WardenMotionPolish` graph; `ABP_WardenCrouchReload` and the earlier `ABP_WardenBlindFire` and `ABP_WardenMotion` graphs remain preserved.

The current pass reuses prefab Epic motion and Quaternius CC0 source. No manually keyed replacement motions were authored. Imported or retargeted content is not automatically an accepted gameplay animation: live poses, transitions and deformation must be recorded in `TEST_MATRIX.md`.

## Source and retarget setup

- Unreal centimeters, Z up, actor forward +X; skeletal component yaw -90 degrees and standing mesh Z -90 cm. Preserve canonical scale and capsule dimensions.
- Installed Epic rifle/pistol animations already target the canonical skeleton. The project builds separate eight-direction standing walk/jog Blend Spaces, with source samples at 180 and 450 cm/s, plus their armed idle.
- The acquired Quaternius Universal Animation Library Standard v3 contains 43 clips. Its original in-place GLB and license are retained under `ArtSource/Quaternius`; imported source packages are separate under `Content/Gunner/Animation/Source/Quaternius`.
- `IK_Quaternius`, `IK_Manny` and `RTG_Quaternius_Manny` live under `Content/Gunner/Animation/Rigs`. The authoring script applies Unreal humanoid chain definitions and full-body IK, maps shared chains, aligns the target pose chain-to-chain and applies target foot grounding. Reference bone samples, chain maps, durations and output samples are written to a local retarget report.
- The seven generated Manny candidates are `A_UAL_Crouch_Idle_Loop`, `A_UAL_Crouch_Fwd_Loop`, `A_UAL_Sprint_Loop`, `A_UAL_Punch_Jab`, `A_UAL_Punch_Cross`, `A_UAL_Roll` and `A_UAL_Sword_Attack`. Source/target duration and skeleton binding are checked during authoring. Only actual live verification can accept foot contact, shoulders, hands or transitions.
- All current retarget outputs disable animation-track root motion and force root lock. CharacterMovement owns displacement; the dodge's native movement source is described below. No root-motion vault/warping action is enabled.

## Composed graph and action ownership

`UGunnerAnimInstance` snapshots character velocity, actor-relative forward/right speeds, grounded/stance state, aim pitch, sprint, cover and selected weapon. The graph consumes this state; it does not invent combat state.

The base graph selects rifle/pistol standing locomotion, the real crouch idle/forward Blend Space, a distinct sprint clip and armed airborne loops. Crouch uses speed magnitude because the character turns toward travel. `bDirectionalCrouchReady` stays false: crouched ADS is stationary, and fixed-facing backward/sideways crouch is not faked by playing a forward clip.

Epic rifle/pistol Aim Offsets and an `UpperBody` montage slot blend from `spine_01` over the lower-body pose. The upper layer fades out during sprint, airborne movement and non-ADS crouch. Keeping the imported crouch torso intact avoids raising its head with the upright armed overlay; standing and crouched ADS restore the armed layer. This changes blend weight, not source animation tracks. A separate `FullBody` slot layers the jab/cross actions over locomotion; gameplay prevents movement and incompatible actions during it. The project registers `UpperBody` in the Weapon slot group and `FullBody` in the Traversal group on the canonical skeleton. That metadata change does not reauthor Epic animation tracks.

Fire, reload and equip montages use the installed Epic handling clips. Melee alternates the retargeted `Punch_Jab` and `Punch_Cross` montages. `AM_DodgeRoll` uses the genuine retargeted `Roll` in the FullBody slot at rate scale 1.65. A guarded native CharacterMovement MoveTo root-motion source supplies up to 350 cm of displacement over the montage duration; the acquired skeletal animation remains in-place. The standing capsule, route floor checks, action guards and interruption cleanup are owned by `UGunnerDodgeComponent`. Sword attack remains a dormant candidate. No sword/knife mechanic is enabled. The blind-fire graph adds arm/hand IK only during low-cover blind fire. There is no runtime foot IK in the project graph; hand/foot alignment outside blind fire still requires visual inspection.

Reload uses the protective arm composition described below during crouch and low-cover attachment. Native guards require its readiness flag so a previous graph cannot award ammunition for an invisible action. Equip and dry fire use the protective handling layer described below and require their own readiness flags in actual/pending crouch or low cover. Crouch input rejects active reload/equip. Standing high-cover reload remains available.

Current gun visuals attach to `hand_r` with per-weapon authored grip transforms derived from the actual armed idle. Muzzle offsets are stored in each Weapon Data Asset. Dedicated `weapon_r`, `weapon_back`, `muzzle` and `grip_l` sockets remain future authoring work. Static weapon models do not provide independently animated magazines or bolts.

Gameplay is authoritative. Reload transfers ammunition on a validated, uninterrupted montage-ended callback; stale callbacks are rejected with an action serial and timeouts clear locks without committing ammo. Each melee variant's timed impact request additionally checks the current montage/state and performs guarded geometry queries. Future notifies may request commits but must never own ammo/damage directly. Animation-track root-motion vault/warping remains disabled until its montage, collision path, destination, cancellation and later replication are proven. The current roll uses the bounded native movement source described above.

## Procedural low-cover blind fire

`ABP_WardenBlindFire` reuses the real Quaternius crouch and Epic armed source clips. A separate arm-only layer begins at `clavicle_l` and `clavicle_r`; root, pelvis, spine, neck and head retain the crouch pose. Native two-bone IK raises both hands without stretching, with hand rotations derived from one rigid weapon transform and the existing right-hand attachment. Each weapon's left grip is sampled from its own Epic ADS idle by `Tools/install_blind_fire.py`, not estimated from a generic gun.

The existing mesh-space additive fire montages evaluate after the IK pose so recoil remains visible; the final arm mask prevents the upright torso from replacing protected crouch. `BlindFireAlpha` blends the arm presentation while native combat waits for the actual evaluated hands/muzzle/head, valid cover and clearance. `bBlindFirePoseReady` and per-weapon `bBlindFireGripReady` keep old/unconfigured compositions disabled. The installer creates a new graph, refuses overwrites and unexpected previous classes, and backs up the assigned character and weapon assets before changes. No Epic or Quaternius animation tracks are modified or new keyframes authored. Live reach, protection and firing acceptance belongs in TEST_MATRIX.

## Coverage ledger

| Requirement | Current source/composition | Remaining acceptance / limitation |
|---|---|---|
| Standing armed idle/walk/jog | Epic rifle/pistol eight-direction clips and project Blend Spaces | Inspect both weapons, all directions and transitions in real gameplay |
| Airborne | Epic takeoff, fall loops and additive landing recovery | Project-owned copies lock root; CharacterMovement owns displacement; interruption and rendered pose checked |
| Crouch idle/forward travel | Two real retargeted Quaternius clips; upright armed torso fades out outside ADS | Inspect pelvis/feet/head clearance and blending; body faces travel; no fixed-facing directional set |
| Crouched ADS | Armed upper-body layer over real crouch base | Stationary only until directional crouch coverage is integrated |
| Sprint | Distinct retargeted Quaternius Sprint_Loop | Inspect blending and slide; combat lowered, forward movement only |
| Aim/fire/reload/equip | Epic clips, Aim Offsets and authored montages | Reload uses an arm-only layer while crouched or behind low cover; equip and dry fire also use protective arm composition; static gun mechanics remain simple |
| Standing melee | Alternating retargeted Punch_Jab/Punch_Cross full-body montages | Inspect impact timing/contact; crouched melee disabled; no knife motion |
| Low-cover blind fire | Genuine crouch + arm-only native IK + Epic grip/fire clips | Actual hands, muzzle, head and shot geometry must pass live guards; stationary, low-cover only; source blind-fire clips remain unacquired |
| Cover | Attachment with armed/crouch movement; high-cover ADS physically steps out/returns using directional armed gait | Inspect exposure and return; dedicated enter/exit/wall-lean/corner animations absent; crouched edge-peek disabled; full M3 not accepted |
| Dodge roll | Retargeted Roll, FullBody montage and native guarded movement source | Live roll, bounded travel, wall/stance rejection and interruption checks passed; contact/transition polish remains |
| Vault | No enabled montage/action | Free UAL2 ClimbUp_1m research is not a through-vault |
| Hit/incapacity/death | Installed candidates and source research only | No implemented life-state animation graph |
| Second-source retarget | Source/target rigs, retarget asset and seven outputs authored | Six selected clips exercised in game; sword remains dormant. Complete production transition/deformation coverage is still required |

The current sandbox passes 92 live checks in each of two rendered PIE sessions, including protected head height, evaluated reload/melee slots and cover/roll cancellation. Selected gameplay captures and precise limits are in [TEST_MATRIX](TEST_MATRIX.md). This is bounded sandbox acceptance, not acceptance of the entire ledger.

## Further assets and acceptance

[ASSET_RESEARCH](ASSET_RESEARCH.md) records exact free coverage and optional direct-creator paid tiers. The full UAL1 catalogue has directional crouch candidates; UAL2 Source has a safety-vault candidate. Neither acquisition nor import would by itself provide all dedicated cover-shooter animation work. No paid pack was purchased. No uncertain-license GaspFix or repackaged Mixamo material was imported.

Before enabling another motion, record source/version/license, preserve editable files, retarget to the canonical skeleton and inspect scale, root drift, foot contact, pelvis height, shoulder twist, weapon-hand contact and interruption behavior from the actual camera. Build, launch, exercise and record the result. Missing animation-dependent actions remain gated; do not substitute capsule-only crouch, relabel a climb as a vault or declare M1/M3 complete from static screenshots.

## Crouched reload composition

`ABP_WardenCrouchReload` retains the blind-fire graph and adds a protective arm-only layer for the existing Epic rifle/pistol reload montages. Crouch pelvis, spine, head and legs remain from the acquired Quaternius motion. The reload arm motion is reused without editing source tracks or pinning the hands with blind-fire IK. This is project-authored composition, not an acquired dedicated crouched-reload clip. A graph readiness flag gates protected reloads; the guarded installer preserves the previous graph and backs up the character assignment. Live pose, ammunition and interruption acceptance is recorded in TEST_MATRIX.

## Motion-polish composition

`ABP_WardenMotionPolish` preserves prior blind-fire/reload behavior and adds equip/dry-fire arm blending from each clavicle. `CrouchHandlingAlpha` contributes to `ProtectiveArmsWeight`; genuine crouch still supplies the pelvis, spine, head and legs. Existing Epic equip clips supply handling motion without blind-fire IK pinning the hands. Low-cover held ADS is suppressed during equip and resumes after completion. Interrupted equip releases the action but retains the incoming selected weapon, matching the existing equip contract.

Weapon Data Assets select `AM_Rifle/Pistol_JumpStart` and `JumpLand` on FullBody. Takeoff comes from Epic Jump_Start; landing uses Jump_RecoveryAdditive on actual ground contact. Copies under `Animation/InPlace` disable animation root motion and force the reference-pose root lock; Epic originals are preserved. CharacterMovement remains the displacement authority. Fall loops bridge the phases. Crouch, sprint, cover, dodge, firing and handling actions can interrupt recovery without locking movement.

`AM_Rifle/Pistol_DryFire` uses UpperBody. Empty trigger feedback is rate-limited to 0.8 seconds and creates no ammo/damage commit or exclusive gameplay action. Holding an empty trigger produces one presentation; reload replaces it immediately. `AM_MeleeCross` uses FullBody and alternates with the jab on accepted presses. Cross impact is requested at 28% of its duration; jab retains 38%. Both use the same authority, collision, interruption and single-hit guards. These are standing punches with the equipped weapon retained, not knife attacks.
