"""Read-only persisted asset gate for the local installed traversal composition.

Run after install_installed_traversal.py in a fresh UE 5.8 commandlet. Does not
compile, save, duplicate, assign or enable gameplay assets. Writes only a Saved
report. Rendered behavior/weapon-hand/foot acceptance belongs to native probes.
"""
from pathlib import Path
import hashlib
import json
import math
import os
import subprocess
import unreal as u

P = Path(u.Paths.project_dir()).resolve()
LOCAL = '/Game/Gunner/LicensedLocal/MoverTraversal'
MOTION = '/Game/Gunner/Motion'
SOURCE = '/MoverExamples/Characters/Mannequins/Animations/Manny'
REPORT = P / 'Saved/installed_traversal_asset_validation.json'
CLIPS = {
    'MM_Unarmed_Crouch_Idle': 'A_CrouchIdle',
    'MM_Unarmed_Crouch_Entry': 'A_CrouchEntry',
    'MM_Unarmed_Crouch_Exit': 'A_CrouchExit',
    'MM_Unarmed_Crouch_Walk_Fwd': 'A_CrouchFwd',
    'MM_Unarmed_Crouch_Walk_Bwd': 'A_CrouchBwd',
    'MM_Unarmed_Crouch_Walk_Left': 'A_CrouchLeft',
    'MM_Unarmed_Crouch_Walk_Right': 'A_CrouchRight',
    'VaultOver': 'A_VaultOverCandidate',
}


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def load(path):
    asset = u.load_asset(path)
    require(asset is not None, 'Missing required asset: ' + path)
    return asset


def xyz(value):
    return [value.x, value.y, value.z]


def raw_track_digest(sequence):
    model = sequence.get_editor_property('data_model_interface')
    require(model is not None, 'Editable data model absent: ' + sequence.get_path_name())
    names = [str(name) for name in model.get_bone_track_names()]
    require(names and len(names) == model.get_num_bone_tracks(), 'Invalid bone track inventory')
    tracks = []
    for name in names:
        transforms = u.GunnerAnimationBuilder.get_editable_bone_track_transforms(sequence, name)
        require(len(transforms) == model.get_number_of_keys(), 'Incomplete editable frame data: ' + name)
        tracks.append({'name': name, 'keys': [
            [*xyz(t.translation), t.rotation.x, t.rotation.y, t.rotation.z, t.rotation.w,
             *xyz(t.scale3d)] for t in transforms]})
    rate = model.get_frame_rate()
    data = {'frames': model.get_number_of_frames(), 'keys': model.get_number_of_keys(),
            'frame_rate': [rate.numerator, rate.denominator], 'tracks': tracks}
    # Read each original editable frame through the supported data-model API.
    # This verifies all bone transforms at source frame times, not curve tangents.
    encoded = json.dumps(data, sort_keys=True, separators=(',', ':'), allow_nan=False).encode()
    return {'sha256': hashlib.sha256(encoded).hexdigest(), 'tracks': len(tracks),
            'frames': data['frames'], 'keys': data['keys'], 'frame_rate': data['frame_rate'],
            'representation': 'editable_bone_transforms_at_every_source_frame'}


def check_original_package(relative):
    expected = subprocess.run(['git', 'rev-parse', 'HEAD:' + relative], cwd=P,
                              check=True, capture_output=True, text=True).stdout.strip()
    current = subprocess.run(['git', 'hash-object', '--', relative], cwd=P,
                             check=True, capture_output=True, text=True).stdout.strip()
    require(current == expected, 'Original committed package changed: ' + relative)
    return current


def main():
    authoring = json.loads((P / 'Saved/installed_traversal_authoring_report.json').read_text())
    require(authoring['status'] == 'locally_installed_pending_live_validation',
            'Local authoring did not finish')
    inspection = json.loads((P / 'Saved/installed_traversal_inventory.json').read_text())
    character = load(MOTION + '/Characters/BP_WardenMotion')
    cdo = u.get_default_object(character.generated_class())
    component = cdo.get_component_by_class(u.SkeletalMeshComponent)
    graph = load(LOCAL + '/ABP_WardenTraversal')
    require(component.get_editor_property('anim_class') == graph.generated_class(),
            'Persisted character does not use the local traversal graph')
    status = str(graph.get_editor_property('status'))
    require('UP_TO_DATE' in status and 'WARNINGS' not in status,
            'Graph is not compiled without warnings: ' + status)
    defaults = u.get_default_object(graph.generated_class())
    for field in ('directional_crouch_pose_ready', 'blind_fire_pose_ready',
                  'crouch_reload_pose_ready', 'crouch_equip_pose_ready', 'crouch_dry_fire_pose_ready'):
        require(defaults.get_editor_property(field), 'Graph capability missing: ' + field)
    settings = cdo.get_editor_property('motion_settings')
    require(settings.get_path_name() == LOCAL + '/DA_MotionTraversal.DA_MotionTraversal',
            'Persisted character does not use local motion settings')
    require(settings.get_editor_property('directional_crouch_ready') and
            settings.get_editor_property('contextual_traversal'), 'Local movement gates are disabled')
    speed = settings.get_editor_property('crouch_speed')
    skeleton = load('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple').get_editor_property('skeleton')
    report = {'status': 'validating', 'graph_status': status, 'copies': {},
              'original_package_git_blobs': {}, 'limitations':
              ['Static asset gate only; native rendered probes must validate gait, grip and cover protection.']}
    for relative in ('Content/Gunner/Motion/Animation/BS_Crouch.uasset',
                     'Content/Gunner/Animation/Retargeted/Manny/A_UAL_Crouch_Idle_Loop.uasset',
                     'Content/Gunner/Animation/Retargeted/Manny/A_UAL_Crouch_Fwd_Loop.uasset',
                     'Content/Gunner/Motion/Movement/DA_MotionSettings.uasset'):
        report['original_package_git_blobs'][relative] = check_original_package(relative)

    content = Path(u.Paths.convert_relative_path_to_full(u.Paths.engine_plugins_dir())) / \
        'Experimental/MoverExamples/Content'
    relative_content = os.path.relpath(content, Path(u.Paths.convert_relative_path_to_full(''))).replace(os.sep, '/') + '/'
    require('"' not in relative_content and '\n' not in relative_content, 'Unsupported engine path')
    mounted_before = u.PluginBlueprintLibrary.is_plugin_mounted('MoverExamples')
    enabled_before = set(u.PluginBlueprintLibrary.get_enabled_plugin_names())
    mounted_here = False
    try:
        if not mounted_before:
            u.SystemLibrary.execute_console_command(None,
                'PackageName.RegisterMountPoint /MoverExamples/ "' + relative_content + '"')
            mounted_here = True
        registry = u.AssetRegistryHelpers.get_asset_registry()
        registry.scan_paths_synchronous([SOURCE, LOCAL], force_rescan=True)
        for name, output in CLIPS.items():
            original_file = content / 'Characters/Mannequins/Animations/Manny' / (name + '.uasset')
            require(hashlib.sha256(original_file.read_bytes()).hexdigest() ==
                    inspection['source_files'][name]['sha256'], 'Installed source changed: ' + name)
            original = load(SOURCE + '/' + name)
            copy = load(LOCAL + '/' + output)
            require(copy.get_editor_property('skeleton') == skeleton, 'Copy is not canonical: ' + name)
            require(abs(copy.sequence_length - original.sequence_length) < 0.000001,
                    'Copy duration changed: ' + name)
            source_raw = raw_track_digest(original)
            copy_raw = raw_track_digest(copy)
            require(copy_raw == source_raw, 'Editable bone data changed: ' + name)
            root_fields = ('enable_root_motion', 'force_root_lock', 'root_motion_root_lock')
            if name == 'VaultOver':
                require(all(copy.get_editor_property(field) == original.get_editor_property(field)
                            for field in root_fields), 'Unassigned vault root policy changed')
            else:
                require(not copy.get_editor_property('enable_root_motion') and
                        copy.get_editor_property('force_root_lock') and
                        copy.get_editor_property('root_motion_root_lock') == u.RootMotionRootLock.REF_POSE,
                        'Crouch root-lock policy missing: ' + name)
                options = u.AnimPoseEvaluationOptions()
                options.incorporate_root_motion_into_pose = False
                roots = []
                for fraction in (0, .25, .5, .75, 1):
                    pose = u.AnimPoseExtensions.get_anim_pose_at_time(copy, copy.sequence_length * fraction, options)
                    roots.append(xyz(u.AnimPoseExtensions.get_bone_pose(
                        pose, 'root', u.AnimPoseSpaces.WORLD).translation))
                require(max(math.dist(root, roots[0]) for root in roots) < .01,
                        'Locked root drifts in evaluated pose: ' + name)
            expected_rate = speed / 300.0 if '_Walk_' in name else original.get_editor_property('rate_scale')
            require(abs(copy.get_editor_property('rate_scale') - expected_rate) < .000001,
                    'Gait play rate differs from intended travel speed: ' + name)
            report['copies'][name] = {'editable_bone_data': copy_raw, 'duration': copy.sequence_length,
                                      'rate_scale': copy.get_editor_property('rate_scale'),
                                      'root_policy': {field: str(copy.get_editor_property(field)) for field in root_fields}}

        blend = load(LOCAL + '/BS_DirectionalCrouch')
        require(blend.get_editor_property('skeleton') == skeleton, 'Blend Space skeleton mismatch')
        expected = {'A_CrouchIdle': (0, 0), 'A_CrouchFwd': (speed, 0), 'A_CrouchBwd': (-speed, 0),
                    'A_CrouchLeft': (0, -speed), 'A_CrouchRight': (0, speed)}
        actual = {}
        for sample in blend.get_editor_property('sample_data'):
            name = sample.get_editor_property('animation').get_name()
            value = sample.get_editor_property('sample_value')
            actual[name] = (value.x, value.y)
        require(actual == expected and len(blend.get_editor_property('sample_data')) == 5,
                'Directional Blend Space does not contain five correct signed samples')
        report['blend_samples'] = actual
        node_class = u.load_class(None, '/Script/AnimGraph.AnimGraphNode_BlendSpacePlayer')
        graph_blends = [node.get_editor_property('node').get_editor_property('blend_space').get_path_name()
                        for node in u.AnimationLibrary.get_nodes_of_class(graph, node_class, True)]
        require(blend.get_path_name() in graph_blends and
                load(MOTION + '/Animation/BS_Crouch').get_path_name() in graph_blends,
                'Graph lost directional crouch or the separate protective crouch source')
        report['graph_blend_spaces'] = graph_blends
        # Native rendered probes exercise the protective selector, signed axes and
        # rate input. The AnimGraph-only query cannot accept K2 variable nodes.
        report['graph_input_validation'] = 'native traversal probe required'
        dependency_options = u.AssetRegistryDependencyOptions()
        dependency_options.include_soft_package_references = True
        dependency_options.include_hard_package_references = True
        for path in authoring['local_only_outputs']:
            dependencies = [str(item) for item in registry.get_dependencies(path, dependency_options)]
            require(not any(item.startswith('/MoverExamples/') for item in dependencies),
                    'Local asset still requires mounted plugin packages: ' + path)
            if path.endswith('/ABP_WardenTraversal'):
                require(not any('A_VaultOverCandidate' in item for item in dependencies),
                        'Vault candidate was unexpectedly activated in the graph')
        require(set(u.PluginBlueprintLibrary.get_enabled_plugin_names()) == enabled_before,
                'Validation changed plugin enablement')
        report['status'] = 'persisted_asset_gate_passed'
        REPORT.write_text(json.dumps(report, indent=2) + '\n')
        u.log('GUNNER_INSTALLED_TRAVERSAL_ASSETS_PASS copies=8 signed_samples=5 protected_source_preserved=true')
    finally:
        if mounted_here:
            u.SystemLibrary.execute_console_command(None,
                'PackageName.UnregisterMountPoint /MoverExamples/ "' + relative_content + '"')


if __name__ == '__main__':
    main()
