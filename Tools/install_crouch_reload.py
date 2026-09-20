"""Add protective crouched reload composition without changing source clips.

Creates ABP_WardenCrouchReload once, retaining the previous blind-fire graph.
The generated graph uses the existing UpperBody reload montages through a
clavicle-only arm mask; real crouch supplies the pelvis, spine, neck and head.
Run after compiling GunnerEditor with no other Unreal process open.
"""
from pathlib import Path
import datetime
import hashlib
import json
import shutil
import unreal as u

PROJECT = Path(u.Paths.project_dir()).resolve()
ROOT = '/Game/Gunner/Motion'
ANIMS = '/Game/Characters/Mannequins/Anims'
NEW_GRAPH = ROOT + '/Animation/ABP_WardenCrouchReload'
OLD_CLASS = ROOT + '/Animation/ABP_WardenBlindFire.ABP_WardenBlindFire_C'
CHARACTER = ROOT + '/Characters/BP_WardenMotion'
LIB = u.EditorAssetLibrary


def load(path):
    result = u.load_asset(path)
    if not result:
        raise RuntimeError('Required asset is missing: ' + path)
    return result


def save(asset):
    asset.modify()
    if not LIB.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError('Could not save: ' + asset.get_path_name())


if LIB.does_asset_exist(NEW_GRAPH):
    raise RuntimeError('Crouch-reload graph already exists; refusing to overwrite it')
character = load(CHARACTER)
defaults = u.get_default_object(character.generated_class())
mesh_component = defaults.get_component_by_class(u.SkeletalMeshComponent)
previous_class = mesh_component.get_editor_property('anim_class')
if not previous_class or previous_class.get_path_name() != OLD_CLASS:
    raise RuntimeError('Unexpected animation class; refusing to replace authored composition')
old_anim_defaults = u.get_default_object(previous_class)
if not old_anim_defaults.get_editor_property('blind_fire_pose_ready'):
    raise RuntimeError('Expected working blind-fire graph is not configured')

mesh = load('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
skeleton = mesh.get_editor_property('skeleton')
report = {'old_animation_class': OLD_CLASS, 'new_graph': NEW_GRAPH, 'reloads': {}}
for kind in ('Rifle', 'Pistol'):
    weapon = load(ROOT + '/Weapons/DA_' + kind)
    if not weapon.get_editor_property('blind_fire_grip_ready'):
        raise RuntimeError('Blind-fire grip data must remain configured: ' + kind)
    montage = weapon.get_editor_property('reload_montage')
    expected = ROOT + '/Animation/Montages/AM_' + kind + '_Reload'
    if not montage or montage.get_path_name().split('.')[0] != expected:
        raise RuntimeError('Unexpected reload montage: ' + kind)
    tracks = montage.get_editor_property('slot_anim_tracks')
    if len(tracks) != 1 or str(tracks[0].get_editor_property('slot_name')) != 'UpperBody':
        raise RuntimeError('Reload must use the existing UpperBody action slot: ' + kind)
    source = load(ANIMS + '/' + kind + '/MM_' + kind + '_Reload')
    if source.get_editor_property('skeleton') != skeleton:
        raise RuntimeError('Reload source uses a different skeleton: ' + kind)
    if source.get_editor_property('additive_anim_type') != u.AdditiveAnimationType.AAT_NONE:
        raise RuntimeError('Expected complete, non-additive arm handling source: ' + kind)
    report['reloads'][kind] = {'source': source.get_path_name(),
                             'montage': montage.get_path_name(),
                             'source_length': source.get_editor_property('sequence_length'),
                             'slot': 'UpperBody'}

backup = PROJECT / 'Saved/AuthoringDrafts' / (
    'before-crouch-reload-' + datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
backup.mkdir(parents=True, exist_ok=False)
character_file = PROJECT / 'Content/Gunner/Motion/Characters/BP_WardenMotion.uasset'
shutil.copy2(character_file, backup / character_file.name)
report['backup'] = str(backup)
report['character_sha256_before'] = hashlib.sha256(character_file.read_bytes()).hexdigest()
(backup / 'manifest.json').write_text(json.dumps(report, indent=2))

graph = u.GunnerAnimationBuilder.create_locomotion_blueprint(
    NEW_GRAPH, skeleton, mesh,
    load(ROOT + '/Animation/BS_Rifle'), load(ROOT + '/Animation/BS_Pistol'),
    load(ANIMS + '/Rifle/Jump/MM_Rifle_Jump_Fall_Loop'),
    load(ANIMS + '/Pistol/Jump/MM_Pistol_Jump_Fall_Loop'),
    load(ROOT + '/Animation/BS_Crouch'),
    load('/Game/Gunner/Animation/Retargeted/Manny/A_UAL_Sprint_Loop'),
    load(ANIMS + '/Rifle/AIM/AO_Rifle'), load(ANIMS + '/Pistol/Aim/AO_Pistol'), True, True)
if not graph or not graph.generated_class():
    raise RuntimeError('Crouch-reload graph failed to compile; old character assignment retained')
anim_defaults = u.get_default_object(graph.generated_class())
if not anim_defaults.get_editor_property('blind_fire_pose_ready') or not anim_defaults.get_editor_property('crouch_reload_pose_ready'):
    raise RuntimeError('Generated graph did not enable both validated composition gates')
save(graph)

# Compilation can replace generated component templates. Reacquire defaults
# before assigning and saving; never compile again after this assignment.
u.BlueprintEditorLibrary.compile_blueprint(character)
defaults = u.get_default_object(character.generated_class())
mesh_component = defaults.get_component_by_class(u.SkeletalMeshComponent)
defaults.modify()
mesh_component.modify()
character.modify()
mesh_component.set_anim_instance_class(graph.generated_class())
if mesh_component.get_editor_property('anim_class') != graph.generated_class():
    raise RuntimeError('Character did not accept the new animation class')
save(character)
report['assigned_class'] = mesh_component.get_editor_property('anim_class').get_path_name()
report['blind_fire_pose_ready'] = bool(anim_defaults.get_editor_property('blind_fire_pose_ready'))
report['crouch_reload_pose_ready'] = bool(anim_defaults.get_editor_property('crouch_reload_pose_ready'))
report['character_sha256_after'] = hashlib.sha256(character_file.read_bytes()).hexdigest()
(PROJECT / 'Saved/crouch_reload_authoring_report.json').write_text(json.dumps(report, indent=2))
u.log('GUNNER_CROUCH_RELOAD_INSTALLED ' + json.dumps(report))
