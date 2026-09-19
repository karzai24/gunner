"""Author the Quaternius-to-Manny retarget setup and a small motion shortlist once.

Run in UE 5.8 Editor-Cmd with -run=pythonscript -script=<this path> -NullRHI.
Existing rigs or output clips cause an early refusal. Source imports and the
canonical skeleton are read only; corrections live in the IK Retargeter asset.
"""
from pathlib import Path
import json
import math
import unreal as u

PROJECT = Path(u.Paths.project_dir()).resolve()
SOURCE = '/Game/Gunner/Animation/Source/Quaternius/UAL1_Standard/SkeletalMeshes'
RIGS = '/Game/Gunner/Animation/Rigs'
OUTPUT = '/Game/Gunner/Animation/Retargeted/Manny'
SHORTLIST = ('Crouch_Idle_Loop', 'Crouch_Fwd_Loop', 'Sprint_Loop',
             'Punch_Jab', 'Punch_Cross', 'Roll', 'Sword_Attack')
SOURCE_MESH = SOURCE + '/UAL1_Standard'
TARGET_MESH = '/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple'


def load(path):
    result = u.load_asset(path)
    if not result:
        raise RuntimeError('Missing required asset: ' + path)
    return result


def new_asset(name, directory, cls, factory):
    result = u.AssetToolsHelpers.get_asset_tools().create_asset(name, directory, cls, factory)
    if not result:
        raise RuntimeError('Could not create asset ' + directory + '/' + name)
    return result


def vec(v):
    return [round(v.x, 4), round(v.y, 4), round(v.z, 4)]


def bone_positions(pose, names):
    output = {}
    for name in names:
        transform = u.AnimPoseExtensions.get_bone_pose(pose, name, u.AnimPoseSpaces.WORLD)
        translation = transform.translation
        if not all(math.isfinite(v) for v in (translation.x, translation.y, translation.z)):
            raise RuntimeError('Non-finite evaluated bone: ' + name)
        q = transform.rotation
        output[name] = {'position': vec(translation), 'scale': vec(transform.scale3d),
                        'rotation_xyzw': [round(q.x, 5), round(q.y, 5), round(q.z, 5), round(q.w, 5)]}
    return output


def inspect_sequence(sequence):
    options = u.AnimPoseEvaluationOptions()
    length = sequence.get_editor_property('sequence_length')
    samples = []
    for fraction in (0.0, 0.25, 0.5, 0.75, 1.0):
        pose = u.AnimPoseExtensions.get_anim_pose_at_time(sequence, length * fraction, options)
        samples.append({'time': round(length * fraction, 4),
                        'bones': bone_positions(pose, ['root', 'pelvis', 'head',
                                                       'hand_l', 'hand_r', 'foot_l', 'foot_r'])})
    return {'path': sequence.get_path_name(), 'length': length,
            'skeleton': sequence.get_editor_property('skeleton').get_path_name(), 'samples': samples}


# Report installed candidate properties even if a later authoring step rejects an existing output.
stock = {}
for relative in ('Rifle/MM_Rifle_Fire', 'Rifle/MM_Rifle_Reload',
                 'Pistol/MM_Pistol_Fire', 'Pistol/MM_Pistol_Reload',
                 'Unarmed/Attack/MM_Attack_01'):
    asset = load('/Game/Characters/Mannequins/Anims/' + relative)
    stock[relative] = {'length': asset.get_editor_property('sequence_length'),
                       'additive': str(asset.get_editor_property('additive_anim_type')),
                       'reference_type': str(asset.get_editor_property('ref_pose_type'))}
    u.log('GUNNER_STOCK_CLIP ' + relative + ' ' + json.dumps(stock[relative]))
for relative in ('Rifle/AIM/AO_Rifle', 'Pistol/Aim/AO_Pistol'):
    asset = load('/Game/Characters/Mannequins/Anims/' + relative)
    parameters = asset.get_editor_property('blend_parameters')
    stock[relative] = {'class': asset.get_class().get_name(),
                      'axes': [{'name': p.get_editor_property('display_name'),
                                'min': p.get_editor_property('min'),
                                'max': p.get_editor_property('max')} for p in parameters],
                      'samples': [{'clip': s.get_editor_property('animation').get_path_name(),
                                   'position': vec(s.get_editor_property('sample_value'))}
                                  for s in asset.get_editor_property('sample_data')]}
    u.log('GUNNER_STOCK_AIM ' + relative + ' ' + json.dumps(stock[relative]))
target_mesh = load(TARGET_MESH)
target_skeleton = target_mesh.get_editor_property('skeleton')
stock['sockets'] = [str(name) for name in u.AnimPoseExtensions.get_socket_names(
    u.AnimPoseExtensions.get_reference_pose(target_skeleton))]
u.log('GUNNER_STOCK_SOCKETS ' + json.dumps(stock['sockets']))
for weapon in ('SM_Rifle', 'SM_Pistol'):
    candidates = [path for path in u.EditorAssetLibrary.list_assets('/Game/Weapons', recursive=True)
                  if path.rsplit('.', 1)[-1] == weapon]
    if candidates:
        mesh = load(candidates[0])
        box = mesh.get_bounding_box()
        component = u.StaticMeshComponent()
        component.set_static_mesh(mesh)
        stock[weapon] = {'path': mesh.get_path_name(), 'min': vec(box.min), 'max': vec(box.max),
                         'sockets': [str(name) for name in component.get_all_socket_names()]}
idle = load('/Game/Characters/Mannequins/Anims/Rifle/MF_Rifle_Idle_ADS')
pose = u.AnimPoseExtensions.get_anim_pose_at_time(idle, 0.0, u.AnimPoseEvaluationOptions())
stock['rifle_idle_hand_r'] = bone_positions(pose, ['hand_r'])
(PROJECT / 'Saved/motion_stock_inventory.json').write_text(json.dumps(stock, indent=2))

expected = [RIGS + '/IK_Quaternius', RIGS + '/IK_Manny', RIGS + '/RTG_Quaternius_Manny']
expected += [OUTPUT + '/A_UAL_' + name for name in SHORTLIST]
for path in expected:
    if u.EditorAssetLibrary.does_asset_exist(path):
        raise RuntimeError('Refusing to overwrite authored asset: ' + path)

source_mesh = load(SOURCE_MESH)
source_skeleton = source_mesh.get_editor_property('skeleton')
source_sequences = [load(SOURCE + '/UAL1_Standard' + name) for name in SHORTLIST]
report = {'source_mesh': SOURCE_MESH, 'target_mesh': TARGET_MESH,
          'source_clips': [], 'target_clips': [], 'rigs': {}}


def make_rig(name, mesh):
    rig = new_asset(name, RIGS, u.IKRigDefinition, u.IKRigDefinitionFactory())
    controller = u.IKRigController.get_controller(rig)
    if not controller.set_skeletal_mesh(mesh):
        raise RuntimeError('IK Rig rejected mesh: ' + mesh.get_path_name())
    recognized = controller.apply_auto_generated_retarget_definition()
    if not recognized:
        raise RuntimeError('Skeleton did not match an Unreal humanoid definition; manual mapping required')
    ik_created = controller.apply_auto_fbik()
    if not ik_created:
        raise RuntimeError('Could not create humanoid full-body IK setup')
    chains = controller.get_retarget_chains()
    reference = u.AnimPoseExtensions.get_reference_pose(mesh.get_editor_property('skeleton'))
    report['rigs'][name] = {
        'auto_definition': recognized, 'auto_fbik': ik_created,
        'pelvis': str(controller.get_retarget_root()),
        'root_motion_bone': str(controller.get_root_motion_bone()),
        'chains': [{'name': str(c.chain_name),
                    'start': str(controller.get_retarget_chain_start_bone(c.chain_name)),
                    'end': str(controller.get_retarget_chain_end_bone(c.chain_name))}
                   for c in chains],
        'reference_bones': bone_positions(reference, ['root', 'pelvis', 'head',
                                                     'hand_l', 'hand_r', 'foot_l', 'foot_r'])}
    u.log('GUNNER_RIG ' + name + ' ' + json.dumps(report['rigs'][name]))
    return rig


source_rig = make_rig('IK_Quaternius', source_mesh)
target_rig = make_rig('IK_Manny', target_mesh)
retargeter = new_asset('RTG_Quaternius_Manny', RIGS, u.IKRetargeter, u.IKRetargetFactory())
controller = u.IKRetargeterController.get_controller(retargeter)
controller.set_ik_rig(u.RetargetSourceOrTarget.SOURCE, source_rig)
controller.set_ik_rig(u.RetargetSourceOrTarget.TARGET, target_rig)
controller.set_preview_mesh(u.RetargetSourceOrTarget.SOURCE, source_mesh)
controller.set_preview_mesh(u.RetargetSourceOrTarget.TARGET, target_mesh)
controller.add_default_ops()
# This shortlist contains skeletal tracks only. Remove the optional curves op so
# reruns in a fresh project do not warn about absent facial/animation curves.
for index in reversed(range(controller.get_num_retarget_ops())):
    if str(controller.get_op_name(index)) == 'Remap Curves':
        controller.remove_retarget_op(index)
source_chains = {c['name'] for c in report['rigs']['IK_Quaternius']['chains']}
for chain in report['rigs']['IK_Manny']['chains']:
    if chain['name'] in source_chains:
        controller.set_source_chain(chain['name'], chain['name'])
controller.auto_align_all_bones(u.RetargetSourceOrTarget.TARGET,
                                u.RetargetAutoAlignMethod.CHAIN_TO_CHAIN)
controller.snap_bone_to_ground('foot_l', u.RetargetSourceOrTarget.TARGET)
report['ops'] = [str(controller.get_op_name(i)) for i in range(controller.get_num_retarget_ops())]
u.log('GUNNER_RETARGET_OPS ' + json.dumps(report['ops']))

inputs = u.IKRetargetBatchOperationInputs()
inputs.assets_to_retarget = [u.EditorAssetLibrary.find_asset_data(asset.get_path_name())
                            for asset in source_sequences]
inputs.source_mesh = source_mesh
inputs.target_mesh = target_mesh
inputs.ik_retarget_asset = retargeter
inputs.search = 'UAL1_Standard'
inputs.replace = 'A_UAL_'
inputs.target_path = OUTPUT
inputs.use_source_path = False
inputs.include_referenced_assets = False
inputs.overwrite_existing_files = False
outputs = u.IKRetargetBatchOperation.run_batch_retarget(inputs)
if len(outputs) != len(SHORTLIST):
    raise RuntimeError(f'Retarget output count {len(outputs)} != {len(SHORTLIST)}')

for name, source in zip(SHORTLIST, source_sequences):
    target = load(OUTPUT + '/A_UAL_' + name)
    if target.get_editor_property('skeleton') != target_skeleton:
        raise RuntimeError('Retarget did not bind the canonical skeleton: ' + name)
    if abs(target.get_editor_property('sequence_length') - source.get_editor_property('sequence_length')) > 0.001:
        raise RuntimeError('Retarget changed animation duration: ' + name)
    # These are in-place locomotion/actions. Explicitly preserve the capsule as movement authority.
    target.set_editor_property('enable_root_motion', False)
    target.set_editor_property('force_root_lock', True)
    report['source_clips'].append(inspect_sequence(source))
    report['target_clips'].append(inspect_sequence(target))
    if not u.EditorAssetLibrary.save_loaded_asset(target, only_if_is_dirty=False):
        raise RuntimeError('Could not save retargeted sequence: ' + name)
    u.log('GUNNER_RETARGET_CLIP ' + target.get_path_name())
for asset in (source_rig, target_rig, retargeter):
    if not u.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError('Could not save retarget setup: ' + asset.get_path_name())

(PROJECT / 'Saved/motion_retarget_report.json').write_text(json.dumps(report, indent=2))
u.log('GUNNER_RETARGET_COMPLETE count=' + str(len(outputs)))
