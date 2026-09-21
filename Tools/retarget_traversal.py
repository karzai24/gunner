"""Retarget ONLY UAL2 Slide_Start/Loop/Exit candidates to canonical Manny.

Run after import_traversal_source.py with UE 5.8 Editor-Cmd, -run=pythonscript
and -NullRHI. Separate source/target rigs avoid assumptions about UAL1 identity.
Refuses existing outputs. This does not assign an AnimBP, montage or gameplay action.
"""
from pathlib import Path
import hashlib
import json
import math
import unreal as u

PROJECT = Path(u.Paths.project_dir()).resolve()
SOURCE = '/Game/Gunner/Animation/Source/Quaternius/UAL2_Standard/SkeletalMeshes'
SOURCE_MESH = SOURCE + '/UAL2_Standard'
TARGET_MESH = '/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple'
RIGS = '/Game/Gunner/Animation/Rigs'
OUTPUT = '/Game/Gunner/Animation/Retargeted/Manny'
SOURCE_RIG_NAME = 'IK_QuaterniusUAL2'
TARGET_RIG_NAME = 'IK_MannyTraversal'
RETARGETER_NAME = 'RTG_QuaterniusUAL2_Manny'
SHORTLIST = {'Slide_Start': 25 / 30, 'Slide_Loop': 2.0, 'Slide_Exit': 0.5}
SOURCE_FILE = ('ArtSource/Quaternius/UAL2_Standard_v2_1/source/'
               'Universal Animation Library 2[Standard]/Unreal-Godot/UAL2_Standard.glb')
SOURCE_SHA256 = '8cee20ab1bc55130092447e810e26df22dd2803eccc54f52137a7d54d7ab88a8'
SAMPLE_BONES = ('root', 'pelvis', 'head', 'hand_l', 'hand_r', 'foot_l', 'foot_r')
REQUIRED_CHAINS = {'Spine', 'Neck', 'Head', 'LeftLeg', 'RightLeg',
                   'LeftFoot', 'RightFoot', 'LeftClavicle', 'RightClavicle',
                   'LeftArm', 'RightArm'}
REPORT = PROJECT / 'Saved/traversal_retarget_report.json'


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def load(path, cls):
    asset = u.load_asset(path)
    require(asset is not None and isinstance(asset, cls), 'Missing/wrong asset: ' + path)
    return asset


def vector(value):
    return [round(value.x, 5), round(value.y, 5), round(value.z, 5)]


def sample_bones(pose):
    actual = {str(name).lower() for name in u.AnimPoseExtensions.get_bone_names(pose)}
    require(set(SAMPLE_BONES) <= actual, 'Pose does not include all required evaluated bones')
    result = {}
    for name in SAMPLE_BONES:
        transform = u.AnimPoseExtensions.get_bone_pose(pose, name, u.AnimPoseSpaces.WORLD)
        rotation = transform.rotation
        raw = (transform.translation.x, transform.translation.y, transform.translation.z,
               transform.scale3d.x, transform.scale3d.y, transform.scale3d.z,
               rotation.x, rotation.y, rotation.z, rotation.w)
        require(all(math.isfinite(value) for value in raw), 'Non-finite bone transform: ' + name)
        result[name] = {'position': vector(transform.translation),
                        'scale': vector(transform.scale3d),
                        'rotation_xyzw': [round(value, 6) for value in
                                          (rotation.x, rotation.y, rotation.z, rotation.w)]}
    return result


def inspect_sequence(sequence, respect_root_lock=False):
    length = sequence.get_editor_property('sequence_length')
    require(math.isfinite(length) and length > 0, 'Invalid sequence length')
    options = u.AnimPoseEvaluationOptions()
    # The default true value tells the evaluator to ignore the asset's root lock.
    options.incorporate_root_motion_into_pose = not respect_root_lock
    samples = []
    for fraction in (0.0, 0.1, 0.25, 0.5, 0.75, 0.9, 1.0):
        pose = u.AnimPoseExtensions.get_anim_pose_at_time(sequence, length * fraction, options)
        samples.append({'time': round(length * fraction, 6), 'bones': sample_bones(pose)})
    return {'path': sequence.get_path_name(), 'length': length,
            'skeleton': sequence.get_editor_property('skeleton').get_path_name(),
            'enable_root_motion': sequence.get_editor_property('enable_root_motion'),
            'force_root_lock': sequence.get_editor_property('force_root_lock'),
            'root_motion_root_lock': str(sequence.get_editor_property('root_motion_root_lock')),
            'evaluated_with_root_lock': respect_root_lock,
            'samples': samples}


def create(name, cls, factory):
    asset = u.AssetToolsHelpers.get_asset_tools().create_asset(name, RIGS, cls, factory)
    require(asset is not None, 'Cannot create new asset: ' + name)
    return asset


def make_rig(name, mesh, report):
    rig = create(name, u.IKRigDefinition, u.IKRigDefinitionFactory())
    controller = u.IKRigController.get_controller(rig)
    require(controller.set_skeletal_mesh(mesh), 'IK Rig rejected actual mesh: ' + name)
    require(controller.apply_auto_generated_retarget_definition(),
            'Actual skeleton does not match the humanoid definition: ' + name)
    require(controller.apply_auto_fbik(), 'Could not generate full-body IK: ' + name)
    chains = [{'name': str(chain.chain_name),
               'start': str(controller.get_retarget_chain_start_bone(chain.chain_name)),
               'end': str(controller.get_retarget_chain_end_bone(chain.chain_name))}
              for chain in controller.get_retarget_chains()]
    require(REQUIRED_CHAINS <= {chain['name'] for chain in chains},
            'Generated rig lacks required humanoid chains: ' + name)
    require(str(controller.get_retarget_root()).lower() == 'pelvis',
            'Unexpected retarget root: ' + name)
    require(str(controller.get_root_motion_bone()).lower() == 'root',
            'Unexpected root motion bone: ' + name)
    reference = u.AnimPoseExtensions.get_reference_pose(mesh.get_editor_property('skeleton'))
    actual_bones = {str(bone).lower() for bone in u.AnimPoseExtensions.get_bone_names(reference)}
    for chain in chains:
        require(chain['start'].lower() in actual_bones and chain['end'].lower() in actual_bones,
                'Retarget chain does not match actual skeleton: ' + str(chain))
    report['rigs'][name] = {'mesh': mesh.get_path_name(),
                           'skeleton': mesh.get_editor_property('skeleton').get_path_name(),
                           'root': str(controller.get_retarget_root()),
                           'root_motion_bone': str(controller.get_root_motion_bone()),
                           'chains': chains, 'reference_bones': sample_bones(reference)}
    return rig


def main():
    expected = [RIGS + '/' + name for name in
                (SOURCE_RIG_NAME, TARGET_RIG_NAME, RETARGETER_NAME)]
    expected += [OUTPUT + '/A_UAL2_' + name for name in SHORTLIST]
    for path in expected:
        require(not u.EditorAssetLibrary.does_asset_exist(path),
                'Refusing to overwrite authored asset: ' + path)
        require(not (PROJECT / 'Content' / (path.removeprefix('/Game/') + '.uasset')).exists(),
                'Expected output package already exists on disk: ' + path)
    require(hashlib.sha256((PROJECT / SOURCE_FILE).read_bytes()).hexdigest() == SOURCE_SHA256,
            'Preserved source hash mismatch')
    import_report = json.loads((PROJECT / 'Saved/traversal_import_inventory.json').read_text())
    require(import_report['source_sha256'] == SOURCE_SHA256,
            'Import report belongs to a different release')
    source_mesh = load(SOURCE_MESH, u.SkeletalMesh)
    target_mesh = load(TARGET_MESH, u.SkeletalMesh)
    source_skeleton = source_mesh.get_editor_property('skeleton')
    target_skeleton = target_mesh.get_editor_property('skeleton')
    require(source_skeleton != target_skeleton, 'Expected distinct source and canonical skeletons')
    require(source_skeleton.get_path_name() == import_report['source_skeleton'],
            'Actual source skeleton differs from import report')
    sources = {}
    report = {'status': 'retarget_candidates_not_gameplay_validated',
              'source_sha256': SOURCE_SHA256, 'source_mesh': SOURCE_MESH,
              'target_mesh': TARGET_MESH, 'source_clips': [], 'target_clips': [], 'rigs': {},
              'limitation': 'Generic UAL2 slides, not dedicated cover slam/lean/vault animations.'}
    for name, duration in SHORTLIST.items():
        sequence = load(SOURCE + '/UAL2_Standard' + name, u.AnimSequence)
        require(sequence.get_editor_property('skeleton') == source_skeleton,
                'Source clip skeleton mismatch: ' + name)
        require(abs(sequence.sequence_length - duration) < 0.001,
                'Unexpected source duration: ' + name)
        sources[name] = sequence
        report['source_clips'].append(inspect_sequence(sequence))

    source_rig = make_rig(SOURCE_RIG_NAME, source_mesh, report)
    target_rig = make_rig(TARGET_RIG_NAME, target_mesh, report)
    retargeter = create(RETARGETER_NAME, u.IKRetargeter, u.IKRetargetFactory())
    controller = u.IKRetargeterController.get_controller(retargeter)
    controller.set_ik_rig(u.RetargetSourceOrTarget.SOURCE, source_rig)
    controller.set_ik_rig(u.RetargetSourceOrTarget.TARGET, target_rig)
    controller.set_preview_mesh(u.RetargetSourceOrTarget.SOURCE, source_mesh)
    controller.set_preview_mesh(u.RetargetSourceOrTarget.TARGET, target_mesh)
    require(controller.get_ik_rig(u.RetargetSourceOrTarget.SOURCE) == source_rig and
            controller.get_ik_rig(u.RetargetSourceOrTarget.TARGET) == target_rig and
            controller.get_preview_mesh(u.RetargetSourceOrTarget.SOURCE) == source_mesh and
            controller.get_preview_mesh(u.RetargetSourceOrTarget.TARGET) == target_mesh,
            'Retargeter did not retain the actual mesh/rig assignments')
    controller.add_default_ops()
    for index in reversed(range(controller.get_num_retarget_ops())):
        if str(controller.get_op_name(index)) == 'Remap Curves':
            controller.remove_retarget_op(index)
    source_chains = {chain['name'] for chain in report['rigs'][SOURCE_RIG_NAME]['chains']}
    mapped = []
    for chain in report['rigs'][TARGET_RIG_NAME]['chains']:
        if chain['name'] in source_chains:
            require(controller.set_source_chain(chain['name'], chain['name']),
                    'Cannot map retarget chain: ' + chain['name'])
            require(str(controller.get_source_chain(chain['name'])) == chain['name'],
                    'Retargeter did not retain chain mapping: ' + chain['name'])
            mapped.append(chain['name'])
    require(REQUIRED_CHAINS <= set(mapped), 'Missing required source/target chain correspondence')
    controller.auto_align_all_bones(u.RetargetSourceOrTarget.TARGET,
                                    u.RetargetAutoAlignMethod.CHAIN_TO_CHAIN)
    controller.snap_bone_to_ground('foot_l', u.RetargetSourceOrTarget.TARGET)
    report['mapped_chains'] = mapped
    report['ops'] = [str(controller.get_op_name(i)) for i in range(controller.get_num_retarget_ops())]

    inputs = u.IKRetargetBatchOperationInputs()
    inputs.assets_to_retarget = [u.EditorAssetLibrary.find_asset_data(asset.get_path_name())
                                for asset in sources.values()]
    inputs.source_mesh = source_mesh
    inputs.target_mesh = target_mesh
    inputs.ik_retarget_asset = retargeter
    inputs.search = 'UAL2_Standard'
    inputs.replace = 'A_UAL2_'
    inputs.target_path = OUTPUT
    inputs.use_source_path = False
    inputs.include_referenced_assets = False
    inputs.overwrite_existing_files = False
    outputs = u.IKRetargetBatchOperation.run_batch_retarget(inputs)
    require(len(outputs) == 3, 'Retarget did not return exactly three candidates')

    for name, source in sources.items():
        target = load(OUTPUT + '/A_UAL2_' + name, u.AnimSequence)
        require(target.get_editor_property('skeleton') == target_skeleton,
                'Retarget did not bind canonical Manny: ' + name)
        require(abs(target.sequence_length - source.sequence_length) < 0.001,
                'Retarget changed duration: ' + name)
        target.set_editor_property('enable_root_motion', False)
        target.set_editor_property('force_root_lock', True)
        target.set_editor_property('root_motion_root_lock', u.RootMotionRootLock.REF_POSE)
        require(not target.get_editor_property('enable_root_motion') and
                target.get_editor_property('force_root_lock') and
                target.get_editor_property('root_motion_root_lock') == u.RootMotionRootLock.REF_POSE,
                'Root policy was not applied: ' + name)
        inspected = inspect_sequence(target, respect_root_lock=True)
        roots = [sample['bones']['root'] for sample in inspected['samples']]
        origin = roots[0]['position']
        root_drift = max(math.dist(root['position'], origin) for root in roots)
        require(root_drift < 0.01, 'Evaluated locked root drifted: ' + name)
        inspected['evaluated_root_drift_cm'] = root_drift
        report['target_clips'].append(inspected)
        require(u.EditorAssetLibrary.save_loaded_asset(target, only_if_is_dirty=False),
                'Could not save candidate: ' + name)
    for asset in (source_rig, target_rig, retargeter):
        require(u.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False),
                'Could not save retarget setup: ' + asset.get_path_name())
    # Confirm no source property/pose changed during retargeting.
    for original, sequence in zip(report['source_clips'], sources.values()):
        require(inspect_sequence(sequence) == original,
                'Source inspection changed during retarget: ' + sequence.get_path_name())
    REPORT.write_text(json.dumps(report, indent=2) + '\n')
    u.log('GUNNER_TRAVERSAL_RETARGET_COMPLETE count=3 root_motion=false root_lock=REF_POSE')


main()
