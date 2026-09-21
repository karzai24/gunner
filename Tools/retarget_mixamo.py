"""Retarget inventoried local Mixamo clips to Manny, without gameplay assignment.

Run only AFTER reviewing Saved/mixamo_import_inventory.json from the separate
import stage. UE 5.8 Editor-Cmd: -run=pythonscript -script=<this file> -NullRHI.
All rigs/sequences remain under ignored /Game/Gunner/LicensedLocal/Mixamo.
Verified completed reports allow appending clips without overwriting assets;
unrecorded/existing outputs or partial runs cause refusal. Source Hips is never locked.
For a pelvis-as-root source, a ground-level Manny root is generated before the
target's dedicated root is locked; crouch height and body lean stay on pelvis.
Derivatives receive empty AssetImportData: the source FBX cannot be reimported
directly into Manny. External inventory/report files retain full provenance.
No AnimBP, montage, Blueprint, settings, input or runtime content is changed.
"""
from pathlib import Path
import importlib.util
import json
import math
import unreal as u

PROJECT = Path(u.Paths.project_dir()).resolve()
helper_spec = importlib.util.spec_from_file_location(
    'gunner_mixamo_import_helpers', PROJECT / 'Tools/import_mixamo_source.py')
h = importlib.util.module_from_spec(helper_spec)
helper_spec.loader.exec_module(h)
require = h.require
LOCAL = h.LOCAL
RIGS = LOCAL + '/Rigs'
OUTPUT = LOCAL + '/Retargeted/Manny'
SOURCE_RIG = 'IK_MixamoXBot'
TARGET_RIG = 'IK_MannyMixamo'
RETARGETER = 'RTG_MixamoXBot_Manny'
TARGET_MESH = '/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple'
REPORT = PROJECT / 'Saved/mixamo_retarget_report.json'
TARGET_BONES = {'root': 'root', 'pelvis': 'pelvis', 'spine': 'spine_01',
                'head': 'head', 'hand_l': 'hand_l', 'hand_r': 'hand_r',
                'foot_l': 'foot_l', 'foot_r': 'foot_r'}
REQUIRED_CHAINS = {'Spine', 'Neck', 'Head', 'LeftLeg', 'RightLeg',
                   'LeftFoot', 'RightFoot', 'LeftClavicle', 'RightClavicle',
                   'LeftArm', 'RightArm'}


def report_write(report):
    temporary = REPORT.with_suffix('.json.tmp')
    temporary.write_text(json.dumps(report, indent=2) + '\n')
    temporary.replace(REPORT)


def package_hashes(path):
    package = path.split('.', 1)[0]
    require(package.startswith(LOCAL + '/'), 'Package outside local Mixamo namespace')
    filename = PROJECT / 'Content' / (package.removeprefix('/Game/') + '.uasset')
    require(filename.is_file(), 'Missing saved package: ' + str(filename))
    return {str(candidate.relative_to(PROJECT)): h.sha256(candidate)
            for extension in ('.uasset', '.uexp', '.ubulk')
            if (candidate := filename.with_suffix(extension)).is_file()}


def require_no_reimport_source(sequence):
    data = sequence.get_editor_property('asset_import_data')
    require(data is not None and data.get_class().get_name() == 'AssetImportData' and
            not data.extract_filenames(), 'Derived Manny clip retains an FBX reimport route: ' + sequence.get_path_name())


def validate_previous(report, source_mesh, target_mesh, entries):
    require(report.get('schema') == 1 and
            report.get('status') == 'retarget_complete_candidates_not_gameplay_validated',
            'Previous retarget did not complete; inspect its report and preserve partial assets before recovery')
    require(report['source_mesh'] == source_mesh.get_path_name() and report['target_mesh'] == TARGET_MESH,
            'Previous retarget uses different meshes')
    require(report.get('target_reference_inventory') == h.skeleton_data(target_mesh),
            'Target reference skeleton changed since previous retarget')
    require(set(report['target_clips']) <= {entry['id'] for entry in entries},
            'Append manifest must retain all previously retargeted clip IDs')
    expected = {RIGS + '/' + name for name in (SOURCE_RIG, TARGET_RIG, RETARGETER)}
    expected.update(OUTPUT + '/A_Mixamo_' + name for name in report['target_clips'])
    actual = {path.split('.', 1)[0] for directory in (RIGS, OUTPUT)
              for path in u.EditorAssetLibrary.list_assets(directory, recursive=True)}
    require(actual == expected, 'Existing local retarget directories contain unrecorded/missing assets')
    require(set(report.get('asset_package_sha256', {})) == expected,
            'Previous report lacks package hashes; refusing unverified asset reuse')
    for path in sorted(expected):
        require(package_hashes(path) == report['asset_package_sha256'][path],
                'Saved retarget package changed; refusing overwrite/reuse: ' + path)
    for name, previous in report['target_clips'].items():
        sequence = h.load(OUTPUT + '/A_Mixamo_' + name, u.AnimSequence)
        require_no_reimport_source(sequence)
        require(h.inspect_sequence(sequence, TARGET_BONES) == previous['raw'] and
                h.inspect_sequence(sequence, TARGET_BONES, respect_root_lock=True) == previous['in_place'],
                'Prior derivative properties/pose changed: ' + name)
    source_rig = h.load(RIGS + '/' + SOURCE_RIG, u.IKRigDefinition)
    target_rig = h.load(RIGS + '/' + TARGET_RIG, u.IKRigDefinition)
    retargeter = h.load(RIGS + '/' + RETARGETER, u.IKRetargeter)
    controller = u.IKRetargeterController.get_controller(retargeter)
    for side, rig, mesh in ((u.RetargetSourceOrTarget.SOURCE, source_rig, source_mesh),
                             (u.RetargetSourceOrTarget.TARGET, target_rig, target_mesh)):
        require(controller.get_ik_rig(side) == rig and controller.get_preview_mesh(side) == mesh,
                'Recorded rig/mesh assignments changed')
    for name in report['mapped_chains']:
        require(str(controller.get_source_chain(name)) == name, 'Recorded chain mapping changed: ' + name)
    return source_rig, target_rig, retargeter


def new_asset(name, cls, factory):
    asset = u.AssetToolsHelpers.get_asset_tools().create_asset(name, RIGS, cls, factory)
    require(asset is not None, 'Cannot create local retarget asset: ' + name)
    return asset


def make_rig(name, mesh, pelvis, root, report):
    rig = new_asset(name, u.IKRigDefinition, u.IKRigDefinitionFactory())
    controller = u.IKRigController.get_controller(rig)
    require(controller.set_skeletal_mesh(mesh), 'IK Rig rejected inspected mesh: ' + name)
    # UE 5.8 ships a Mixamo hierarchy definition and strips shared name prefixes.
    require(controller.apply_auto_generated_retarget_definition(),
            'Actual skeleton not recognized; inspect imported hierarchy before manual mapping: ' + name)
    require(str(controller.get_retarget_root()).lower() == pelvis.lower(),
            'Auto-generated pelvis does not match actual skeleton: ' + name)
    require(controller.set_root_motion_bone(root), 'Cannot assign actual root-motion bone: ' + name)
    require(controller.apply_auto_fbik(), 'Cannot generate humanoid IK rig: ' + name)
    chains = [{'name': str(chain.chain_name),
               'start': str(controller.get_retarget_chain_start_bone(chain.chain_name)),
               'end': str(controller.get_retarget_chain_end_bone(chain.chain_name))}
              for chain in controller.get_retarget_chains()]
    actual = {str(bone).lower() for bone in u.AnimPoseExtensions.get_bone_names(
        u.AnimPoseExtensions.get_reference_pose(mesh.get_editor_property('skeleton')))}
    require(REQUIRED_CHAINS <= {chain['name'] for chain in chains},
            'Auto rig lacks required chains: ' + name)
    require(all(chain['start'].lower() in actual and chain['end'].lower() in actual for chain in chains),
            'Generated chains refer to absent bones: ' + name)
    report['rigs'][name] = {'mesh': mesh.get_path_name(),
                           'skeleton': mesh.get_editor_property('skeleton').get_path_name(),
                           'pelvis': str(controller.get_retarget_root()),
                           'root_motion_bone': str(controller.get_root_motion_bone()),
                           'chains': chains}
    return rig


def configure_retargeter(source_mesh, target_mesh, source_rig, target_rig, source_bones, report):
    asset = new_asset(RETARGETER, u.IKRetargeter, u.IKRetargetFactory())
    controller = u.IKRetargeterController.get_controller(asset)
    for side, rig, mesh in ((u.RetargetSourceOrTarget.SOURCE, source_rig, source_mesh),
                             (u.RetargetSourceOrTarget.TARGET, target_rig, target_mesh)):
        controller.set_ik_rig(side, rig)
        controller.set_preview_mesh(side, mesh)
        require(controller.get_ik_rig(side) == rig and controller.get_preview_mesh(side) == mesh,
                'Retargeter did not retain actual source/target assignments')
    controller.add_default_ops()
    for index in reversed(range(controller.get_num_retarget_ops())):
        if str(controller.get_op_name(index)) == 'Remap Curves':
            controller.remove_retarget_op(index)
    source_chains = {chain['name'] for chain in report['rigs'][SOURCE_RIG]['chains']}
    mapped = []
    for target_chain in report['rigs'][TARGET_RIG]['chains']:
        name = target_chain['name']
        source_name = name if name in source_chains else 'None'
        require(controller.set_source_chain(source_name, name), 'Cannot set exact chain map: ' + name)
        require(str(controller.get_source_chain(name)) == source_name, 'Chain mapping did not persist: ' + name)
        if source_name != 'None':
            mapped.append(name)
    require(REQUIRED_CHAINS <= set(mapped), 'Incomplete source/target limb correspondence')
    controller.auto_align_all_bones(u.RetargetSourceOrTarget.TARGET,
                                    u.RetargetAutoAlignMethod.CHAIN_TO_CHAIN)
    # The engine's auto-retarget workflow excludes biped feet from auto alignment.
    target_pose = controller.get_current_retarget_pose_name(u.RetargetSourceOrTarget.TARGET)
    controller.reset_retarget_pose(target_pose, ['foot_l', 'foot_r'], u.RetargetSourceOrTarget.TARGET)
    controller.snap_bone_to_ground('foot_l', u.RetargetSourceOrTarget.TARGET)
    root_ops = []
    report['ops'] = []
    for index in range(controller.get_num_retarget_ops()):
        op_controller = controller.get_op_controller(index)
        if isinstance(op_controller, u.IKRetargetRunIKRigController):
            # Mirrors UE's auto-retarget: authored FK/pelvis first; auto-FBIK is
            # retained for adjustment but not accepted blindly for all candidates.
            require(controller.set_retarget_op_enabled(index, False), 'Cannot disable provisional auto-IK solve')
        if isinstance(op_controller, u.IKRetargetRootMotionController):
            root_ops.append(index)
            op_controller.set_source_root_bone(source_bones['root'])
            op_controller.set_target_root_bone('root')
            op_controller.set_target_pelvis_bone('pelvis')
            settings = op_controller.get_settings()
            if source_bones['root'] == source_bones['pelvis']:
                settings.set_editor_property('root_motion_source', u.RootMotionSource.GENERATE_FROM_TARGET_PELVIS)
                settings.set_editor_property('root_height_source', u.RootMotionHeightSource.SNAP_TO_GROUND)
                # For in-place derivatives, keep pelvis rotations/body lean in
                # the pose; extracting pelvis tilt into a locked root loses it.
                settings.set_editor_property('rotate_with_pelvis', False)
                settings.set_editor_property('maintain_offset_from_pelvis', False)
            op_controller.set_settings(settings)
            persisted = op_controller.get_settings()
            if source_bones['root'] == source_bones['pelvis']:
                require(persisted.get_editor_property('root_motion_source') ==
                        u.RootMotionSource.GENERATE_FROM_TARGET_PELVIS and
                        persisted.get_editor_property('root_height_source') ==
                        u.RootMotionHeightSource.SNAP_TO_GROUND and
                        not persisted.get_editor_property('rotate_with_pelvis'),
                        'Pelvis-as-root safe retarget settings did not persist')
            report['root_generator'] = {
                'source_root': str(op_controller.get_source_root_bone()),
                'target_root': str(op_controller.get_target_root_bone()),
                'target_pelvis': str(op_controller.get_target_pelvis_bone()),
                'source': str(persisted.get_editor_property('root_motion_source')),
                'height': str(persisted.get_editor_property('root_height_source')),
                'rotate_with_pelvis': persisted.get_editor_property('rotate_with_pelvis'),
                'maintain_offset_from_pelvis': persisted.get_editor_property('maintain_offset_from_pelvis')}
        report['ops'].append({'name': str(controller.get_op_name(index)),
                               'controller': op_controller.get_class().get_name() if op_controller else None,
                               'enabled': controller.get_retarget_op_enabled(index)})
    require(len(root_ops) == 1, 'Expected one inspectable dedicated-root generation op')
    report['mapped_chains'] = mapped
    return asset


def validate_lock(raw, locked):
    require(locked['root_max_displacement_cm'] < 0.01, 'Manny root moved despite in-place lock')
    # Ground-generated roots must not carry pelvis height or pitch/roll away.
    max_height_change = max(abs(left['bones']['pelvis']['position'][2] -
                                right['bones']['pelvis']['position'][2])
                            for left, right in zip(raw['samples'], locked['samples']))
    require(max_height_change < 0.05,
            'Target root lock erased vertical pose; reject this retarget configuration')
    require(locked['pelvis_height_above_lowest_foot_bone_cm'][0] > 0,
            'Retarget pelvis is at/below feet; inspect before accepting')
    return {'root_drift_cm': locked['root_max_displacement_cm'],
            'max_pelvis_height_change_when_locking_cm': max_height_change}


def main():
    h.assert_local_ignored()
    imported = json.loads(h.REPORT.read_text())
    require(imported.get('schema') == 1 and imported.get('status') == 'import_complete_not_retargeted',
            'Complete and review the source inventory before retargeting')
    source_entry, entries = h.read_manifest()
    require(entries, 'No animation candidates in manifest')
    require(source_entry['sha256'] == imported['source']['sha256'], 'Manifest source mesh changed')
    source_mesh = h.load(imported['skeleton_inventory']['mesh'], u.SkeletalMesh)
    h.check_source_import_path(source_mesh, 'XBot')
    require(h.sha256(h.PRESERVED_SOURCE / 'XBot.fbx') == source_entry['sha256'],
            'Preserved source mesh FBX changed')
    require(h.skeleton_data(source_mesh) == imported['skeleton_inventory'], 'Source reference skeleton changed')
    source_bones = imported['skeleton_inventory']['sample_bones']
    require(set(TARGET_BONES) <= set(source_bones), 'Actual source bone aliases are incomplete; review inventory')
    target_mesh = h.load(TARGET_MESH, u.SkeletalMesh)
    target_skeleton = target_mesh.get_editor_property('skeleton')
    target_reference = u.AnimPoseExtensions.get_reference_pose(target_skeleton)
    actual_target = {str(name).lower() for name in u.AnimPoseExtensions.get_bone_names(target_reference)}
    require(set(TARGET_BONES.values()) <= actual_target, 'Canonical Manny skeleton is incomplete')
    require(source_mesh.get_editor_property('skeleton') != target_skeleton, 'Expected distinct source skeleton')
    previous_report = json.loads(REPORT.read_text()) if REPORT.is_file() else None
    if previous_report is None:
        h.require_empty_directory(RIGS)
        h.require_empty_directory(OUTPUT)
    sources = {}
    source_before = {}
    for entry in entries:
        name = entry['id']
        record = imported['animations'].get(name)
        require(record and record['source']['sha256'] == entry['sha256'], 'Missing/stale import record: ' + name)
        require(h.sha256(h.PRESERVED_SOURCE / (name + '.fbx')) == entry['sha256'],
                'Preserved source FBX changed: ' + name)
        sequence = h.load(record['sequence']['path'], u.AnimSequence)
        h.check_source_import_path(sequence, name)
        require(sequence.get_editor_property('skeleton') == source_mesh.get_editor_property('skeleton'),
                'Source clip skeleton differs from inspected source mesh: ' + name)
        require(not sequence.get_editor_property('enable_root_motion') and
                not sequence.get_editor_property('force_root_lock'), 'Source Hips/root is locked: ' + name)
        inspected = h.inspect_sequence(sequence, source_bones)
        require(inspected == record['sequence'], 'Source clip changed since import: ' + name)
        sources[name], source_before[name] = sequence, inspected
    if previous_report is not None:
        require(previous_report['source_mesh_sha256'] == source_entry['sha256'],
                'Previously retargeted source mesh hash changed')
        source_rig, target_rig, retargeter = validate_previous(previous_report, source_mesh, target_mesh, entries)
        for name in previous_report['target_clips']:
            require(source_before[name] == previous_report['source_clips'][name],
                    'Previously retargeted source clip changed: ' + name)
    report = previous_report or {'schema': 1, 'status': 'started', 'source_mesh': source_mesh.get_path_name(),
              'target_mesh': TARGET_MESH, 'source_mesh_sha256': source_entry['sha256'],
              'target_reference_inventory': h.skeleton_data(target_mesh),
              'source_clips': source_before, 'target_clips': {}, 'rigs': {},
              'asset_package_sha256': {},
              'limitation': 'Animation candidates only; no assignment, gameplay/weapon grip/cover-clearance acceptance.',
              'root_policy': 'Source Hips unlocked; generated ground root locked only on Manny; CharacterMovement owns displacement.',
              'reimport_policy': 'Manny derivatives have empty AssetImportData; source FBX remains outside Content in Saved/LicensedSources/Mixamo.',
              'redistribution': 'Local licensed sources and derivatives are git-ignored.'}
    try:
        if previous_report is None:
            source_rig = make_rig(SOURCE_RIG, source_mesh, source_bones['pelvis'], source_bones['root'], report)
            target_rig = make_rig(TARGET_RIG, target_mesh, 'pelvis', 'root', report)
            retargeter = configure_retargeter(source_mesh, target_mesh, source_rig, target_rig, source_bones, report)
        report['status'] = 'retargeting_new_candidates'
        report['source_clips'] = source_before
        report_write(report)
        for name, source in sources.items():
            if name in report['target_clips']:
                continue
            target_name = 'A_Mixamo_' + name
            target_path = OUTPUT + '/' + target_name
            require(not u.EditorAssetLibrary.does_asset_exist(target_path), 'Refusing existing output: ' + target_path)
            inputs = u.IKRetargetBatchOperationInputs()
            inputs.assets_to_retarget = [u.EditorAssetLibrary.find_asset_data(source.get_path_name())]
            inputs.source_mesh = source_mesh
            inputs.target_mesh = target_mesh
            inputs.ik_retarget_asset = retargeter
            inputs.search = source.get_name()
            inputs.replace = target_name
            inputs.target_path = OUTPUT
            inputs.use_source_path = False
            inputs.include_referenced_assets = False
            inputs.overwrite_existing_files = False
            outputs = u.IKRetargetBatchOperation.run_batch_retarget(inputs)
            require(len(outputs) == 1, 'Expected exactly one retarget output: ' + name)
            target = h.load(target_path, u.AnimSequence)
            require(target.get_editor_property('skeleton') == target_skeleton, 'Retarget does not use canonical Manny')
            require(abs(target.sequence_length - source.sequence_length) < 0.001, 'Retarget changed duration: ' + name)
            raw_before_lock = h.inspect_sequence(target, TARGET_BONES)
            # IKRetargetBatchOperation duplicates and repairs the source FBX path.
            # That route would reimport X Bot data straight into Manny, bypassing
            # the IK retargeter. Keep provenance in reports, not active import data.
            require(u.GunnerAnimationBuilder.clear_derived_animation_reimport_source(target),
                    'Cannot clear duplicated source-FBX reimport metadata')
            require_no_reimport_source(target)
            target.set_editor_property('enable_root_motion', False)
            target.set_editor_property('force_root_lock', True)
            target.set_editor_property('root_motion_root_lock', u.RootMotionRootLock.REF_POSE)
            raw = h.inspect_sequence(target, TARGET_BONES)
            locked = h.inspect_sequence(target, TARGET_BONES, respect_root_lock=True)
            lock_checks = validate_lock(raw_before_lock, locked)
            h.save(target)
            require_no_reimport_source(target)
            report['target_clips'][name] = {'raw': raw, 'in_place': locked, 'root_validation': lock_checks}
            report['asset_package_sha256'][target_path] = package_hashes(target_path)
            report_write(report)
        if previous_report is None:
            for asset in (source_rig, target_rig, retargeter):
                h.save(asset)
                path = asset.get_path_name().split('.', 1)[0]
                report['asset_package_sha256'][path] = package_hashes(path)
        else:
            # Existing setup and candidate files are read-only during append.
            for path, hashes in previous_report['asset_package_sha256'].items():
                require(package_hashes(path) == hashes, 'Append changed an existing package: ' + path)
        require(h.skeleton_data(source_mesh) == imported['skeleton_inventory'], 'Retarget changed source reference skeleton')
        for name, source in sources.items():
            require(h.inspect_sequence(source, source_bones) == source_before[name],
                    'Retarget changed source properties/pose: ' + name)
        report['status'] = 'retarget_complete_candidates_not_gameplay_validated'
        report_write(report)
        u.log('GUNNER_MIXAMO_RETARGET_COMPLETE clips=' + str(len(sources)) + ' report=' + str(REPORT))
    except Exception as error:
        report['status'] = 'failed'
        report['error'] = str(error)
        report_write(report)
        raise


if __name__ == '__main__':
    main()
