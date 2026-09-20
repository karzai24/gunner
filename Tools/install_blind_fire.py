"""Create the protective blind-fire graph once; preserve originals before assignment.

Uses existing crouch/armed clips and native arm IK. No animation tracks are keyed,
baked or overwritten. Gameplay gates still require the evaluated raised pose.
Run only after compiling GunnerEditor, with no other Unreal process open.
"""
from pathlib import Path
import datetime
import hashlib
import json
import math
import shutil
import unreal as u

PROJECT = Path(u.Paths.project_dir()).resolve()
ROOT = '/Game/Gunner/Motion'
ANIMS = '/Game/Characters/Mannequins/Anims'
NEW_GRAPH = ROOT + '/Animation/ABP_WardenBlindFire'
CHARACTER = ROOT + '/Characters/BP_WardenMotion'
OLD_CLASS = ROOT + '/Animation/ABP_WardenMotion.ABP_WardenMotion_C'
LIB = u.EditorAssetLibrary


def load(path):
    asset = u.load_asset(path)
    if not asset:
        raise RuntimeError('Missing required asset: ' + path)
    return asset


def save(asset):
    asset.modify()
    if not LIB.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError('Could not save: ' + asset.get_path_name())


def vector(value):
    return [round(value.x, 5), round(value.y, 5), round(value.z, 5)]


def transform(value):
    q = value.rotation
    return {'translation': vector(value.translation), 'scale': vector(value.scale3d),
            'rotation_xyzw': [q.x, q.y, q.z, q.w]}


def pose(clip):
    return u.AnimPoseExtensions.get_anim_pose_at_time(clip, 0.0, u.AnimPoseEvaluationOptions())


def bone(anim_pose, name):
    result = u.AnimPoseExtensions.get_bone_pose(anim_pose, name, u.AnimPoseSpaces.WORLD)
    if not all(math.isfinite(v) for v in vector(result.translation)):
        raise RuntimeError('Invalid sampled bone: ' + name)
    return result


if LIB.does_asset_exist(NEW_GRAPH):
    raise RuntimeError('Blind-fire graph already exists; refusing to overwrite authored content')
character = load(CHARACTER)
cdo = u.get_default_object(character.generated_class())
component = cdo.get_component_by_class(u.SkeletalMeshComponent)
old_class = component.get_editor_property('anim_class')
if not old_class or old_class.get_path_name() != OLD_CLASS:
    raise RuntimeError('Unexpected current animation Blueprint; refusing reassignment')

mesh = load('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
skeleton = mesh.get_editor_property('skeleton')
report = {'previous_animation_class': OLD_CLASS, 'new_graph': NEW_GRAPH, 'weapons': {}}
definitions = {}
grips = {}
for kind in ('Rifle', 'Pistol'):
    definition = load(ROOT + '/Weapons/DA_' + kind)
    if definition.get_editor_property('blind_fire_grip_ready'):
        raise RuntimeError('Blind-fire grip is already authored: ' + kind)
    if str(definition.get_editor_property('hand_socket')) != 'hand_r':
        raise RuntimeError('Unexpected weapon attachment bone: ' + kind)
    idle = load(ANIMS + '/' + kind + '/MF_' + kind + '_Idle_ADS')
    if idle.get_editor_property('skeleton') != skeleton:
        raise RuntimeError('Armed idle is not on the canonical skeleton: ' + kind)
    sampled = pose(idle)
    right = bone(sampled, 'hand_r')
    left = bone(sampled, 'hand_l')
    held_weapon = u.MathLibrary.compose_transforms(definition.get_editor_property('grip_transform'), right)
    left_relative = u.MathLibrary.make_relative_transform(left, held_weapon)
    reconstructed = u.MathLibrary.compose_transforms(left_relative, held_weapon)
    error = (reconstructed.translation - left.translation).length()
    if error > 0.01:
        raise RuntimeError('Left grip derivation did not reconstruct the original pose: ' + kind)
    definitions[kind] = definition
    grips[kind] = left_relative
    report['weapons'][kind] = {'source_idle': idle.get_path_name(),
                             'hand_r': transform(right), 'hand_l': transform(left),
                             'left_hand_relative_to_weapon': transform(left_relative),
                             'reconstruction_error_cm': error}

crouch = load('/Game/Gunner/Animation/Retargeted/Manny/A_UAL_Crouch_Idle_Loop')
sampled_crouch = pose(crouch)
report['reference_crouch_bones'] = {name: transform(bone(sampled_crouch, name)) for name in
    ('pelvis', 'head', 'clavicle_l', 'upperarm_l', 'lowerarm_l', 'hand_l',
     'clavicle_r', 'upperarm_r', 'lowerarm_r', 'hand_r')}

stamp = datetime.datetime.now().strftime('%Y%m%d-%H%M%S')
backup = PROJECT / 'Saved/AuthoringDrafts' / ('before-blind-fire-' + stamp)
backup.mkdir(parents=True, exist_ok=False)
report['backup'] = str(backup)
manifest = {}
for relative in ('Characters/BP_WardenMotion', 'Weapons/DA_Rifle', 'Weapons/DA_Pistol'):
    source = PROJECT / ('Content/Gunner/Motion/' + relative + '.uasset')
    target = backup / (relative + '.uasset')
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, target)
    manifest[relative] = hashlib.sha256(source.read_bytes()).hexdigest()
(backup / 'manifest.json').write_text(json.dumps(manifest, indent=2))

graph = u.GunnerAnimationBuilder.create_locomotion_blueprint(
    NEW_GRAPH, skeleton, mesh,
    load(ROOT + '/Animation/BS_Rifle'), load(ROOT + '/Animation/BS_Pistol'),
    load(ANIMS + '/Rifle/Jump/MM_Rifle_Jump_Fall_Loop'),
    load(ANIMS + '/Pistol/Jump/MM_Pistol_Jump_Fall_Loop'),
    load(ROOT + '/Animation/BS_Crouch'),
    load('/Game/Gunner/Animation/Retargeted/Manny/A_UAL_Sprint_Loop'),
    load(ANIMS + '/Rifle/AIM/AO_Rifle'), load(ANIMS + '/Pistol/Aim/AO_Pistol'), True)
if not graph or not graph.generated_class():
    raise RuntimeError('Blind-fire graph creation or compilation failed; originals remain assigned')
anim_defaults = u.get_default_object(graph.generated_class())
anim_defaults.modify()
anim_defaults.set_editor_property('blind_fire_pose_ready', True)
save(graph)
for kind, definition in definitions.items():
    definition.set_editor_property('left_hand_grip_transform', grips[kind])
    definition.set_editor_property('blind_fire_grip_ready', True)
    save(definition)

u.BlueprintEditorLibrary.compile_blueprint(character)
updated = u.get_default_object(character.generated_class()).get_component_by_class(u.SkeletalMeshComponent)
updated.modify()
u.get_default_object(character.generated_class()).modify()
character.modify()
updated.set_anim_instance_class(graph.generated_class())
if updated.get_editor_property('anim_class') != graph.generated_class():
    raise RuntimeError('Blueprint did not retain the new animation class')
save(character)
(PROJECT / 'Saved/blind_fire_authoring_report.json').write_text(json.dumps(report, indent=2))
u.log('GUNNER_BLIND_FIRE_GRAPH_INSTALLED ' + json.dumps(report))
