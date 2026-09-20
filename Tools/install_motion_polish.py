"""One-time motion polish composition. Never overwrites an existing authored output.

Reuses Epic and Quaternius clips. Jump copies disable track root motion and lock
root translation; native CharacterMovement remains the only jump displacement.
"""
from pathlib import Path
import datetime
import json
import shutil
import unreal as u

P = Path(u.Paths.project_dir()).resolve()
ROOT = '/Game/Gunner/Motion'
ANIMS = '/Game/Characters/Mannequins/Anims'
GRAPH = ROOT + '/Animation/ABP_WardenMotionPolish'
CHAR = ROOT + '/Characters/BP_WardenMotion'
OLD = ROOT + '/Animation/ABP_WardenCrouchReload.ABP_WardenCrouchReload_C'
LIB = u.EditorAssetLibrary


def load(path):
    a = u.load_asset(path)
    if not a:
        raise RuntimeError('Missing asset: ' + path)
    return a


def save(a):
    a.modify()
    if not LIB.save_loaded_asset(a, only_if_is_dirty=False):
        raise RuntimeError('Save failed: ' + a.get_path_name())
    return a


outputs = [GRAPH, ROOT + '/Animation/Montages/AM_MeleeCross']
for kind in ('Rifle', 'Pistol'):
    outputs += [ROOT + '/Animation/Montages/AM_' + kind + '_' + action
                for action in ('DryFire', 'JumpStart', 'JumpLand')]
    outputs += [ROOT + '/Animation/InPlace/A_' + kind + '_' + action for action in ('JumpStart', 'JumpLand')]
if any(LIB.does_asset_exist(path) for path in outputs):
    raise RuntimeError('A polish output already exists; refusing to overwrite authored content')
character = load(CHAR)
component = u.get_default_object(character.generated_class()).get_component_by_class(u.SkeletalMeshComponent)
if component.get_editor_property('anim_class').get_path_name() != OLD:
    raise RuntimeError('Unexpected current animation class; refusing reassignment')
backup = P / 'Saved/AuthoringDrafts' / ('before-motion-polish-' + datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
backup.mkdir(parents=True, exist_ok=False)
for name in ('Content/Gunner/Motion/Characters/BP_WardenMotion.uasset',
             'Content/Gunner/Motion/Weapons/DA_Rifle.uasset', 'Content/Gunner/Motion/Weapons/DA_Pistol.uasset'):
    shutil.copy2(P / name, backup / Path(name).name)
mesh = load('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
skel = mesh.get_editor_property('skeleton')
report = {'backup': str(backup), 'graph': GRAPH, 'sources': {}, 'weapons': {}}


def montage(name, source, slot='UpperBody', blend_in=0.08, blend_out=0.12):
    if source.get_editor_property('skeleton') != skel:
        raise RuntimeError('Wrong skeleton: ' + source.get_path_name())
    factory = u.AnimMontageFactory()
    factory.set_editor_property('target_skeleton', skel)
    factory.set_editor_property('source_animation', source)
    a = u.AssetToolsHelpers.get_asset_tools().create_asset(name, ROOT + '/Animation/Montages', u.AnimMontage, factory)
    if not a:
        raise RuntimeError('Montage creation failed: ' + name)
    tracks = a.get_editor_property('slot_anim_tracks')
    track = tracks[0]
    track.set_editor_property('slot_name', slot)
    tracks[0] = track
    a.set_editor_property('slot_anim_tracks', tracks)
    for field, duration in (('blend_in', blend_in), ('blend_out', blend_out)):
        value = a.get_editor_property(field)
        value.set_editor_property('blend_time', duration)
        a.set_editor_property(field, value)
    save(a)
    if str(a.get_editor_property('slot_anim_tracks')[0].get_editor_property('slot_name')) != slot:
        raise RuntimeError('Montage slot did not persist')
    return a


cross = montage('AM_MeleeCross', load('/Game/Gunner/Animation/Retargeted/Manny/A_UAL_Punch_Cross'), 'FullBody')
for kind in ('Rifle', 'Pistol'):
    weapon = load(ROOT + '/Weapons/DA_' + kind)
    weapon.modify()
    dry = load(ANIMS + '/' + kind + '/MM_' + kind + '_DryFire')
    weapon.set_editor_property('dry_fire_montage', montage('AM_' + kind + '_DryFire', dry))
    weapon.set_editor_property('alternate_melee_montage', cross)
    weapon.set_editor_property('alternate_melee_impact_fraction', 0.28)
    for action, source_name in (('JumpStart', 'Start'), ('JumpLand', 'RecoveryAdditive')):
        source_path = ANIMS + '/' + kind + '/Jump/MM_' + kind + '_Jump_' + source_name
        source = load(source_path)
        target = ROOT + '/Animation/InPlace/A_' + kind + '_' + action
        clip = LIB.duplicate_asset(source_path, target)
        if not clip:
            raise RuntimeError('Copy failed: ' + target)
        clip.set_editor_property('enable_root_motion', False)
        clip.set_editor_property('force_root_lock', True)
        clip.set_editor_property('root_motion_root_lock', u.RootMotionRootLock.REF_POSE)
        save(clip)
        report['sources'][target] = {'original': source_path, 'length': clip.get_editor_property('sequence_length'),
                                    'additive': str(clip.get_editor_property('additive_anim_type')),
                                    'root_motion': False, 'force_root_lock': True}
        m = montage('AM_' + kind + '_' + action, clip, 'FullBody', 0.04, 0.06 if action == 'JumpStart' else 0.12)
        weapon.set_editor_property('jump_start_montage' if action == 'JumpStart' else 'jump_land_montage', m)
    save(weapon)
    report['weapons'][kind] = weapon.get_path_name()

graph = u.GunnerAnimationBuilder.create_locomotion_blueprint(
    GRAPH, skel, mesh, load(ROOT + '/Animation/BS_Rifle'), load(ROOT + '/Animation/BS_Pistol'),
    load(ANIMS + '/Rifle/Jump/MM_Rifle_Jump_Fall_Loop'), load(ANIMS + '/Pistol/Jump/MM_Pistol_Jump_Fall_Loop'),
    load(ROOT + '/Animation/BS_Crouch'), load('/Game/Gunner/Animation/Retargeted/Manny/A_UAL_Sprint_Loop'),
    load(ANIMS + '/Rifle/AIM/AO_Rifle'), load(ANIMS + '/Pistol/Aim/AO_Pistol'), True, True, True)
if not graph:
    raise RuntimeError('Graph creation failed')
settings = u.get_default_object(graph.generated_class())
for field in ('blind_fire_pose_ready', 'crouch_reload_pose_ready', 'crouch_equip_pose_ready', 'crouch_dry_fire_pose_ready'):
    if not settings.get_editor_property(field):
        raise RuntimeError('Graph capability missing: ' + field)
save(graph)
u.BlueprintEditorLibrary.compile_blueprint(character)
cdo = u.get_default_object(character.generated_class())
component = cdo.get_component_by_class(u.SkeletalMeshComponent)
cdo.modify(); component.modify(); character.modify()
component.set_anim_instance_class(graph.generated_class())
save(character)
report['assigned_class'] = component.get_editor_property('anim_class').get_path_name()
(P / 'Saved/motion_polish_authoring_report.json').write_text(json.dumps(report, indent=2))
u.log('GUNNER_POLISH_INSTALLED ' + json.dumps(report))
