"""Install inspected Epic traversal candidates for local evaluation, once.

UE 5.8 Editor-Cmd -run=pythonscript -script=<absolute path> -NullRHI.
Creates only ignored /Game/Gunner/LicensedLocal assets, plus a recoverable local
BP_WardenMotion assignment. It never enables the Mover gameplay plugins.
Requires the directional/protective graph builder and prior inspection report.
"""
from pathlib import Path
import datetime
import hashlib
import json
import math
import os
import shutil
import subprocess
import unreal as u

P = Path(u.Paths.project_dir()).resolve()
LOCAL = '/Game/Gunner/LicensedLocal/MoverTraversal'
MOTION = '/Game/Gunner/Motion'
ANIMS = '/Game/Characters/Mannequins/Anims'
SOURCE = '/MoverExamples/Characters/Mannequins/Animations/Manny'
CHARACTER = MOTION + '/Characters/BP_WardenMotion'
OLD_CLASS = MOTION + '/Animation/ABP_WardenMotionPolish.ABP_WardenMotionPolish_C'
GRAPH = LOCAL + '/ABP_WardenTraversal'
BLEND = LOCAL + '/BS_DirectionalCrouch'
SETTINGS = LOCAL + '/DA_MotionTraversal'
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
REPORT = P / 'Saved/installed_traversal_authoring_report.json'
LIB = u.EditorAssetLibrary


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def load(path):
    asset = u.load_asset(path)
    require(asset is not None, 'Missing required asset: ' + path)
    return asset


def save(asset):
    asset.modify()
    require(LIB.save_loaded_asset(asset, only_if_is_dirty=False),
            'Could not save: ' + asset.get_path_name())
    return asset


def package_file(path):
    return P / 'Content' / (path.removeprefix('/Game/') + '.uasset')


def main():
    outputs = [GRAPH, BLEND, SETTINGS] + [LOCAL + '/' + name for name in CLIPS.values()]
    for path in outputs:
        require(not LIB.does_asset_exist(path) and not package_file(path).exists(),
                'Refusing to overwrite authored asset: ' + path)
    ignored = subprocess.run(['git', 'check-ignore', '-q', '--no-index',
                              'Content/Gunner/LicensedLocal/MoverTraversal/A_CrouchIdle.uasset'],
                             cwd=P, check=False, capture_output=True)
    require(ignored.returncode == 0, 'LicensedLocal must be ignored before creating derivatives')
    inspection = json.loads((P / 'Saved/installed_traversal_inventory.json').read_text())
    require(inspection['comparison']['equivalent_bone_names_order_hierarchy_reference_pose'],
            'Prior source skeleton inspection did not prove canonical equivalence')
    character = load(CHARACTER)
    cdo = u.get_default_object(character.generated_class())
    component = cdo.get_component_by_class(u.SkeletalMeshComponent)
    require(component.get_editor_property('anim_class').get_path_name() == OLD_CLASS,
            'Unexpected current graph; refusing to replace local authoring')
    old_settings = cdo.get_editor_property('motion_settings')
    require(old_settings and old_settings.get_path_name() ==
            MOTION + '/Movement/DA_MotionSettings.DA_MotionSettings',
            'Unexpected current motion settings; refusing reassignment')
    speed = old_settings.get_editor_property('crouch_speed')
    require(math.isfinite(speed) and 0 < speed <= 300, 'Invalid crouch speed for these 300 cm/s sources')
    mesh = load('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
    skeleton = mesh.get_editor_property('skeleton')
    content = Path(u.Paths.convert_relative_path_to_full(u.Paths.engine_plugins_dir())) / \
        'Experimental/MoverExamples/Content'
    require(content.is_dir(), 'Installed MoverExamples content is unavailable')
    for name in CLIPS:
        source_file = content / 'Characters/Mannequins/Animations/Manny' / (name + '.uasset')
        require(source_file.is_file() and hashlib.sha256(source_file.read_bytes()).hexdigest() ==
                inspection['source_files'][name]['sha256'],
                'Installed source changed since inspection: ' + name)

    backup = P / 'Saved/AuthoringDrafts' / ('before-installed-traversal-' +
                                         datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
    backup.mkdir(parents=True, exist_ok=False)
    original_file = package_file(CHARACTER)
    backup_files = []
    for extension in ('.uasset', '.uexp', '.ubulk'):
        path = original_file.with_suffix(extension)
        if path.exists():
            relative = path.relative_to(P)
            destination = backup / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, destination)
            backup_files.append({'original': str(relative), 'backup': str(destination),
                                 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
    require(backup_files, 'Original character package could not be backed up')
    report = {'status': 'authoring_started', 'backup_directory': str(backup),
              'backup_files': backup_files, 'previous_anim_class': OLD_CLASS,
              'previous_motion_settings': old_settings.get_path_name(),
              'modified_tracked_package': str(original_file.relative_to(P)),
              'local_only_outputs': outputs, 'created': [], 'copies': {},
              'graph': GRAPH, 'motion_settings': SETTINGS,
              'protected_low_cover_source': MOTION + '/Animation/BS_Crouch',
              'activated_candidates': ['crouch idle and four directional travel clips'],
              'unassigned_candidates': ['crouch entry', 'crouch exit', 'VaultOver'],
              'rollback': 'Close Unreal, copy each backup_files entry to its original project-relative path; '
                          'leave ignored local candidates available for inspection. Do not commit this local BP assignment '
                          'without arranging reproduction of LicensedLocal dependencies.',
              'limitations': ['Not gameplay/visual acceptance.',
                              'Vault is a candidate only; no traversal gameplay is enabled by its copy.',
                              'Plugin source/derivatives remain local; no third-party redistribution claim.']}
    REPORT.write_text(json.dumps(report, indent=2) + '\n')

    mounted_before = u.PluginBlueprintLibrary.is_plugin_mounted('MoverExamples')
    enabled_before = set(u.PluginBlueprintLibrary.get_enabled_plugin_names())
    relative_content = os.path.relpath(content, Path(u.Paths.convert_relative_path_to_full(''))).replace(os.sep, '/') + '/'
    require('"' not in relative_content and '\n' not in relative_content, 'Unsupported engine path')
    mounted_here = False
    try:
        if not mounted_before:
            u.SystemLibrary.execute_console_command(None,
                'PackageName.RegisterMountPoint /MoverExamples/ "' + relative_content + '"')
            mounted_here = True
        registry = u.AssetRegistryHelpers.get_asset_registry()
        registry.scan_paths_synchronous([SOURCE, '/MoverExamples/Characters/Mannequins/Meshes'],
                                        force_rescan=True)
        copies = {}
        for name, output_name in CLIPS.items():
            source = load(SOURCE + '/' + name)
            root_lock = name != 'VaultOver'
            target = u.GunnerAnimationBuilder.duplicate_compatible_sequence(
                LOCAL + '/' + output_name, source, skeleton, mesh, root_lock)
            require(target is not None, 'Native compatible copy failed: ' + name)
            require(target.get_editor_property('skeleton') == skeleton and
                    abs(target.sequence_length - source.sequence_length) < 0.000001,
                    'Canonical copy changed skeleton or duration: ' + name)
            if '_Walk_' in name:
                # The inspected source root travels at 300 cm/s in each direction.
                # Root locking removes displacement; rate scaling matches its feet
                # to the CharacterMovement crouch speed without editing raw keys.
                target.set_editor_property('rate_scale', speed / 300.0)
                save(target)
            copies[name] = target
            report['created'].append(target.get_path_name())
            report['copies'][name] = {'source': source.get_path_name(), 'output': target.get_path_name(),
                                      'duration': target.sequence_length,
                                      'rate_scale': target.get_editor_property('rate_scale'),
                                      'root_locked_derivative': root_lock,
                                      'source_sha256': inspection['source_files'][name]['sha256']}
        sequence_names = ('MM_Unarmed_Crouch_Idle', 'MM_Unarmed_Crouch_Walk_Fwd',
                          'MM_Unarmed_Crouch_Walk_Bwd', 'MM_Unarmed_Crouch_Walk_Left',
                          'MM_Unarmed_Crouch_Walk_Right')
        samples = [u.Vector(0, 0, 0), u.Vector(speed, 0, 0), u.Vector(-speed, 0, 0),
                   u.Vector(0, -speed, 0), u.Vector(0, speed, 0)]
        blend = u.GunnerAnimationBuilder.create_directional_blend_space(
            BLEND, skeleton, mesh, [copies[name] for name in sequence_names], samples, speed)
        require(blend is not None, 'Directional crouch Blend Space creation failed')
        report['created'].append(blend.get_path_name())
        graph = u.GunnerAnimationBuilder.create_locomotion_blueprint(
            GRAPH, skeleton, mesh, load(MOTION + '/Animation/BS_Rifle'),
            load(MOTION + '/Animation/BS_Pistol'),
            load(ANIMS + '/Rifle/Jump/MM_Rifle_Jump_Fall_Loop'),
            load(ANIMS + '/Pistol/Jump/MM_Pistol_Jump_Fall_Loop'),
            blend, load('/Game/Gunner/Animation/Retargeted/Manny/A_UAL_Sprint_Loop'),
            load(ANIMS + '/Rifle/AIM/AO_Rifle'), load(ANIMS + '/Pistol/Aim/AO_Pistol'),
            True, True, True, True,
            protective_cover_locomotion=load(MOTION + '/Animation/BS_Crouch'))
        require(graph is not None, 'Traversal animation graph creation failed')
        graph_defaults = u.get_default_object(graph.generated_class())
        for field in ('directional_crouch_pose_ready', 'blind_fire_pose_ready',
                      'crouch_reload_pose_ready', 'crouch_equip_pose_ready', 'crouch_dry_fire_pose_ready'):
            require(graph_defaults.get_editor_property(field), 'Graph capability missing: ' + field)
        save(graph)
        report['created'].append(graph.get_path_name())
        motion = LIB.duplicate_asset(old_settings.get_path_name(), SETTINGS)
        require(motion is not None, 'Motion settings copy failed')
        motion.set_editor_property('directional_crouch_ready', True)
        motion.set_editor_property('contextual_traversal', True)
        save(motion)
        report['created'].append(motion.get_path_name())

        # Reject retained plugin-package references before assigning the character.
        registry.scan_paths_synchronous([LOCAL], force_rescan=True)
        dependency_options = u.AssetRegistryDependencyOptions()
        dependency_options.include_soft_package_references = True
        dependency_options.include_hard_package_references = True
        report['local_dependencies'] = {}
        for path in outputs:
            dependencies = [str(item) for item in registry.get_dependencies(path, dependency_options)]
            require(not any(item.startswith('/MoverExamples/') for item in dependencies),
                    'Local derivative retains a plugin package dependency: ' + path)
            report['local_dependencies'][path] = dependencies
        require(set(u.PluginBlueprintLibrary.get_enabled_plugin_names()) == enabled_before,
                'Installer unexpectedly enabled a gameplay plugin')
        report['status'] = 'assets_ready_before_character_assignment'
        REPORT.write_text(json.dumps(report, indent=2) + '\n')

        u.BlueprintEditorLibrary.compile_blueprint(character)
        cdo = u.get_default_object(character.generated_class())
        component = cdo.get_component_by_class(u.SkeletalMeshComponent)
        cdo.modify(); component.modify(); character.modify()
        cdo.set_editor_property('motion_settings', motion)
        component.set_anim_instance_class(graph.generated_class())
        save(character)
        require(component.get_editor_property('anim_class') == graph.generated_class() and
                cdo.get_editor_property('motion_settings') == motion,
                'Character did not retain local graph/settings assignment')
        report['assigned_anim_class'] = component.get_editor_property('anim_class').get_path_name()
        report['status'] = 'locally_installed_pending_live_validation'
        REPORT.write_text(json.dumps(report, indent=2) + '\n')
        u.log('GUNNER_INSTALLED_TRAVERSAL_AUTHORED copies=8 directional_clips=5 vault_enabled=false')
    finally:
        if mounted_here:
            u.SystemLibrary.execute_console_command(None,
                'PackageName.UnregisterMountPoint /MoverExamples/ "' + relative_content + '"')


main()
