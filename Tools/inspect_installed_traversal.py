"""Inspect installed Epic MoverExamples animation candidates without enabling Mover.

Run in UE 5.8 Editor-Cmd with -run=pythonscript -script=<this file> -NullRHI.
Temporarily mounts the plugin's content using the installed engine console API;
does not load gameplay modules, alter .uproject, duplicate, modify or save assets.
Only the JSON report in Saved is written. No redistribution/license conclusion.
"""
from pathlib import Path
import hashlib
import json
import math
import os
import unreal as u

PROJECT = Path(u.Paths.project_dir()).resolve()
MOUNT = '/MoverExamples/'
CHARACTERS = MOUNT + 'Characters/Mannequins'
ANIMATIONS = CHARACTERS + '/Animations/Manny'
SOURCE_MESH = CHARACTERS + '/Meshes/SKM_Manny_Simple'
TARGET_MESH = '/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple'
CANDIDATES = ('MM_Unarmed_Crouch_Idle', 'MM_Unarmed_Crouch_Entry',
              'MM_Unarmed_Crouch_Exit', 'MM_Unarmed_Crouch_Walk_Fwd',
              'MM_Unarmed_Crouch_Walk_Bwd', 'MM_Unarmed_Crouch_Walk_Left',
              'MM_Unarmed_Crouch_Walk_Right', 'VaultOver')
SAMPLE_BONES = ('root', 'pelvis', 'spine_01', 'spine_03', 'head',
                'hand_l', 'hand_r', 'foot_l', 'foot_r')
REPORT = PROJECT / 'Saved/installed_traversal_inventory.json'


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def vector(value):
    return [round(value.x, 6), round(value.y, 6), round(value.z, 6)]


def transform_data(transform):
    rotation = transform.rotation
    values = (transform.translation.x, transform.translation.y, transform.translation.z,
              transform.scale3d.x, transform.scale3d.y, transform.scale3d.z,
              rotation.x, rotation.y, rotation.z, rotation.w)
    require(all(math.isfinite(value) for value in values), 'Non-finite sampled transform')
    return {'position': vector(transform.translation), 'scale': vector(transform.scale3d),
            'rotation_xyzw': [round(value, 7) for value in
                              (rotation.x, rotation.y, rotation.z, rotation.w)]}


def load(path, cls):
    asset = u.load_asset(path)
    require(asset is not None and isinstance(asset, cls), 'Missing/wrong candidate asset: ' + path)
    return asset


def skeleton_data(mesh):
    skeleton = mesh.get_editor_property('skeleton')
    require(skeleton is not None, 'Mesh has no skeleton: ' + mesh.get_path_name())
    pose = u.AnimPoseExtensions.get_reference_pose(skeleton)
    names = [str(name) for name in u.AnimPoseExtensions.get_bone_names(pose)]
    require(set(SAMPLE_BONES) <= {name.lower() for name in names},
            'Required sample bones absent from ' + skeleton.get_path_name())
    component = u.SkeletalMeshComponent()
    component.set_skinned_asset_and_update(mesh)
    bones = []
    for name in names:
        bones.append({'name': name, 'parent': str(component.get_parent_bone(name)),
                      'local': transform_data(u.AnimPoseExtensions.get_bone_pose(
                          pose, name, u.AnimPoseSpaces.LOCAL)),
                      'world': transform_data(u.AnimPoseExtensions.get_bone_pose(
                          pose, name, u.AnimPoseSpaces.WORLD))})
    return {'mesh': mesh.get_path_name(), 'skeleton': skeleton.get_path_name(),
            'bone_count': len(bones), 'bones': bones}


def compare_skeletons(source, target):
    source_map = {bone['name'].lower(): bone for bone in source['bones']}
    target_map = {bone['name'].lower(): bone for bone in target['bones']}
    common = sorted(source_map.keys() & target_map.keys())
    changed_parents = []
    max_position_error = max_scale_error = max_rotation_error = 0.0
    for name in common:
        left, right = source_map[name], target_map[name]
        if left['parent'].lower() != right['parent'].lower():
            changed_parents.append(name)
        max_position_error = max(max_position_error,
                                 math.dist(left['local']['position'], right['local']['position']))
        max_scale_error = max(max_scale_error,
                              math.dist(left['local']['scale'], right['local']['scale']))
        # q and -q describe the same orientation.
        dot = abs(sum(a * b for a, b in zip(left['local']['rotation_xyzw'],
                                           right['local']['rotation_xyzw'])))
        max_rotation_error = max(max_rotation_error, 1.0 - min(1.0, dot))
    only_source = sorted(source_map.keys() - target_map.keys())
    only_target = sorted(target_map.keys() - source_map.keys())
    order_matches = ([bone['name'].lower() for bone in source['bones']] ==
                     [bone['name'].lower() for bone in target['bones']])
    equivalent = (not only_source and not only_target and not changed_parents and order_matches and
                  max_position_error <= 0.01 and max_scale_error <= 0.00001 and
                  max_rotation_error <= 0.000001)
    return {'same_skeleton_asset': source['skeleton'] == target['skeleton'],
            'equivalent_bone_names_order_hierarchy_reference_pose': equivalent,
            'bone_order_matches': order_matches, 'only_source': only_source,
            'only_target': only_target, 'changed_parents': changed_parents,
            'max_local_position_difference_cm': max_position_error,
            'max_local_scale_difference': max_scale_error,
            'max_local_rotation_one_minus_abs_dot': max_rotation_error,
            'limitation': 'Geometric comparison only; no replacement or gameplay acceptance performed.'}


def sample_sequence(sequence, skeleton):
    require(sequence.get_editor_property('skeleton') == skeleton,
            'Candidate does not use inspected source skeleton: ' + sequence.get_path_name())
    length = sequence.get_editor_property('sequence_length')
    require(math.isfinite(length) and length > 0, 'Invalid candidate duration')
    options = u.AnimPoseEvaluationOptions()
    options.incorporate_root_motion_into_pose = True
    samples = []
    for index in range(61):
        fraction = index / 60
        pose = u.AnimPoseExtensions.get_anim_pose_at_time(sequence, length * fraction, options)
        names = {str(name).lower() for name in u.AnimPoseExtensions.get_bone_names(pose)}
        require(set(SAMPLE_BONES) <= names, 'Sampled pose missing required bones')
        bones = {name: transform_data(u.AnimPoseExtensions.get_bone_pose(
                     pose, name, u.AnimPoseSpaces.WORLD)) for name in SAMPLE_BONES}
        samples.append({'fraction': fraction, 'time': round(length * fraction, 6), 'bones': bones})
    first_root = samples[0]['bones']['root']['position']
    head_above_root = [sample['bones']['head']['position'][2] -
                       sample['bones']['root']['position'][2] for sample in samples]
    pelvis_above_root = [sample['bones']['pelvis']['position'][2] -
                         sample['bones']['root']['position'][2] for sample in samples]
    head_above_lowest_foot = []
    pelvis_above_lowest_foot = []
    feet_above_root = {'foot_l': [], 'foot_r': []}
    for sample in samples:
        bones = sample['bones']
        lowest_foot = min(bones[foot]['position'][2] for foot in ('foot_l', 'foot_r'))
        head_above_lowest_foot.append(bones['head']['position'][2] - lowest_foot)
        pelvis_above_lowest_foot.append(bones['pelvis']['position'][2] - lowest_foot)
        for foot in feet_above_root:
            feet_above_root[foot].append(bones[foot]['position'][2] - bones['root']['position'][2])
    root_relative_bounds = {
        axis: {'min': min(sample['bones']['root']['position'][index] - first_root[index]
                          for sample in samples),
               'max': max(sample['bones']['root']['position'][index] - first_root[index]
                          for sample in samples)}
        for index, axis in enumerate(('x', 'y', 'z'))}
    return {'path': sequence.get_path_name(), 'length': length,
            'skeleton': skeleton.get_path_name(),
            'additive_anim_type': str(sequence.get_editor_property('additive_anim_type')),
            'enable_root_motion': sequence.get_editor_property('enable_root_motion'),
            'force_root_lock': sequence.get_editor_property('force_root_lock'),
            'root_motion_root_lock': str(sequence.get_editor_property('root_motion_root_lock')),
            'raw_root_max_translation_from_first_cm': max(math.dist(
                sample['bones']['root']['position'], first_root) for sample in samples),
            'head_height_above_root_cm': {'min': min(head_above_root), 'max': max(head_above_root)},
            'pelvis_height_above_root_cm': {'min': min(pelvis_above_root), 'max': max(pelvis_above_root)},
            'head_height_above_lowest_foot_bone_cm': {
                'min': min(head_above_lowest_foot), 'max': max(head_above_lowest_foot)},
            'pelvis_height_above_lowest_foot_bone_cm': {
                'min': min(pelvis_above_lowest_foot), 'max': max(pelvis_above_lowest_foot)},
            'foot_bone_height_above_root_cm': {
                name: {'min': min(values), 'max': max(values)} for name, values in feet_above_root.items()},
            'root_relative_bounds_cm': root_relative_bounds,
            'sampling': '61 inclusive raw-pose samples; source root lock intentionally ignored',
            'height_limitation': 'Foot bone origin is not the sole/floor; this does not certify 115 cm cover protection.',
            'samples': samples}


def main():
    content = Path(u.Paths.convert_relative_path_to_full(u.Paths.engine_plugins_dir())) / \
        'Experimental/MoverExamples/Content'
    descriptor = content.parent / 'MoverExamples.uplugin'
    require(descriptor.is_file() and content.is_dir(), 'Installed MoverExamples content not found')
    plugin_data = json.loads(descriptor.read_text())
    source_files = {}
    for name in CANDIDATES:
        path = content / 'Characters/Mannequins/Animations/Manny' / (name + '.uasset')
        require(path.is_file(), 'Installed source file is missing: ' + str(path))
        source_files[name] = {'file': str(path), 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}
    enabled_before = set(u.PluginBlueprintLibrary.get_enabled_plugin_names())
    mounted_before = u.PluginBlueprintLibrary.is_plugin_mounted('MoverExamples')
    # The console API rejects absolute Content paths. Its relative paths resolve
    # against the process BaseDir; ConvertRelativePathToFull uses that same base.
    base = Path(u.Paths.convert_relative_path_to_full(''))
    relative_content = os.path.relpath(content, base).replace(os.sep, '/') + '/'
    require('"' not in relative_content and '\n' not in relative_content,
            'Unsupported quote/newline in engine content path')
    mounted_here = False
    try:
        if not mounted_before:
            u.SystemLibrary.execute_console_command(
                None, 'PackageName.RegisterMountPoint ' + MOUNT + ' "' + relative_content + '"')
            mounted_here = True
        registry = u.AssetRegistryHelpers.get_asset_registry()
        registry.scan_paths_synchronous([ANIMATIONS, CHARACTERS + '/Meshes'], force_rescan=True)
        require(u.EditorAssetLibrary.does_asset_exist(SOURCE_MESH),
                'Content-only mount failed; inspect logs before considering -EnablePlugins=MoverExamples')
        source_mesh = load(SOURCE_MESH, u.SkeletalMesh)
        target_mesh = load(TARGET_MESH, u.SkeletalMesh)
        source = skeleton_data(source_mesh)
        target = skeleton_data(target_mesh)
        source_skeleton = source_mesh.get_editor_property('skeleton')
        clips = [sample_sequence(load(ANIMATIONS + '/' + name, u.AnimSequence), source_skeleton)
                 for name in CANDIDATES]
        options = u.AssetRegistryDependencyOptions()
        options.include_soft_package_references = True
        options.include_hard_package_references = True
        dependencies = {name: [str(path) for path in registry.get_dependencies(
            ANIMATIONS + '/' + name, options)] for name in CANDIDATES}
        enabled_after = set(u.PluginBlueprintLibrary.get_enabled_plugin_names())
        require(enabled_after == enabled_before, 'Inspection unexpectedly enabled a plugin')
        report = {'status': 'source_inspection_only',
                  'engine_version': u.SystemLibrary.get_engine_version(),
                  'plugin_descriptor': str(descriptor),
                  'plugin_description': plugin_data.get('Description'),
                  'plugin_dependencies': plugin_data.get('Plugins'),
                  'mounted_by_inspector': mounted_here,
                  'gameplay_plugins_enabled_before': sorted(enabled_before &
                      {'MoverExamples', 'Mover', 'ChaosMover'}),
                  'plugin_enablement_changed': False,
                  'source_files': source_files, 'source_skeleton': source,
                  'canonical_skeleton': target, 'comparison': compare_skeletons(source, target),
                  'clips': clips, 'package_dependencies': dependencies,
                  'limitations': ['No assets duplicated, mutated, saved or activated.',
                                  'No rendered pose, weapon-grip or collision acceptance.',
                                  'Installed plugin content is not automatically redistributable Examples.']}
        REPORT.write_text(json.dumps(report, indent=2) + '\n')
        u.log('GUNNER_INSTALLED_TRAVERSAL_INSPECTED count=' + str(len(clips)) +
              ' equivalent=' + str(report['comparison'][
                  'equivalent_bone_names_order_hierarchy_reference_pose']))
    finally:
        if mounted_here:
            u.SystemLibrary.execute_console_command(
                None, 'PackageName.UnregisterMountPoint ' + MOUNT + ' "' + relative_content + '"')


main()
