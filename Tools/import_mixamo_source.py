"""Import locally licensed Mixamo FBX sources; never overwrite an existing asset.

Run in the normal editor through Tools/prepare_mixamo_editor.py.
The UE 5.8 FBX mesh importer requires Slate and crashes in a headless commandlet.
Input: Saved/AssetResearch/Mixamo/manifest.json, for example:
{"source": {"file": "X_Bot.fbx", "sha256": "optional expected hash"},
 "animations": [{"id": "CrouchedRun", "file": "CrouchedRun.fbx",
                 "sha256": "optional expected hash", "expected_duration": 1.0}]}
With no manifest, import only X_Bot.fbx to inspect its real bone hierarchy.
FBX copies/import inputs stay in Saved/LicensedSources/Mixamo, outside Content
directory watching. Imported assets stay in git-ignored LicensedLocal. Keeping
FBX beside derived .uassets can trigger automatic reimport into the wrong rig.
Verified imports recorded by this script may be reused to append new clips;
unrecorded/existing outputs are refused, including after an interrupted import.
This script does not retarget, change a character, or enable a gameplay action.
"""
from pathlib import Path
import hashlib
import json
import math
import re
import shutil
import subprocess
import unreal as u

PROJECT = Path(u.Paths.project_dir()).resolve()
RESEARCH = PROJECT / 'Saved/AssetResearch/Mixamo'
MANIFEST = RESEARCH / 'manifest.json'
LOCAL = '/Game/Gunner/LicensedLocal/Mixamo'
LOCAL_DISK = PROJECT / 'Content/Gunner/LicensedLocal/Mixamo'
PRESERVED_SOURCE = PROJECT / 'Saved/LicensedSources/Mixamo'
MESH_DIR = LOCAL + '/Source/XBot'
ANIM_DIR = LOCAL + '/Source/Animations'
REPORT = PROJECT / 'Saved/mixamo_import_inventory.json'
SAMPLE_ROLES = {'pelvis': 'hips', 'spine': 'spine', 'head': 'head',
                'hand_l': 'lefthand', 'hand_r': 'righthand',
                'foot_l': 'leftfoot', 'foot_r': 'rightfoot'}


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_report(report):
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    temporary = REPORT.with_suffix('.json.tmp')
    temporary.write_text(json.dumps(report, indent=2) + '\n')
    temporary.replace(REPORT)


def load(path, cls):
    asset = u.load_asset(path)
    require(asset is not None and isinstance(asset, cls), 'Missing/wrong asset: ' + path)
    return asset


def assert_local_ignored():
    relative = 'Content/Gunner/LicensedLocal/Mixamo/probe.uasset'
    check = subprocess.run(['git', 'check-ignore', '-q', '--', relative], cwd=PROJECT,
                           capture_output=True, text=True)
    require(check.returncode == 0, 'LicensedLocal must be git-ignored before importing')
    tracked = subprocess.run(['git', 'ls-files', '--', 'Content/Gunner/LicensedLocal/Mixamo'],
                             cwd=PROJECT, capture_output=True, text=True, check=True)
    require(not tracked.stdout.strip(), 'Mixamo source/derivatives must not be publicly tracked')
    require(PRESERVED_SOURCE.resolve().is_relative_to((PROJECT / 'Saved').resolve()) and
            not PRESERVED_SOURCE.resolve().is_relative_to((PROJECT / 'Content').resolve()),
            'Preserved FBX directory must stay in Saved and outside Content')
    require(not any(LOCAL_DISK.rglob('*.fbx')), 'Remove/archive old watched Mixamo FBX before importing')


def check_source_import_path(asset, source_id):
    data = asset.get_editor_property('asset_import_data')
    require(data is not None, 'Source asset has no import provenance: ' + asset.get_path_name())
    filenames = [Path(filename).resolve() for filename in data.extract_filenames()]
    expected = (PRESERVED_SOURCE / (source_id + '.fbx')).resolve()
    require(filenames == [expected] and not expected.is_relative_to((PROJECT / 'Content').resolve()),
            'Source import path is not the preserved external FBX: ' + asset.get_path_name())
    return {'class': data.get_class().get_name(), 'filenames': [str(path) for path in filenames]}


def read_manifest():
    manifest = (json.loads(MANIFEST.read_text()) if MANIFEST.is_file() else
                {'source': {'file': 'X_Bot.fbx'}, 'animations': []})
    require(isinstance(manifest, dict) and isinstance(manifest.get('source'), dict) and
            isinstance(manifest.get('animations', []), list), 'Invalid Mixamo manifest schema')
    entries = [dict(manifest['source'], id='XBot')]
    entries += manifest.get('animations', [])
    seen = set()
    for entry in entries:
        name = entry.get('id', '')
        require(re.fullmatch(r'[A-Za-z][A-Za-z0-9_]*', name) is not None,
                'Animation id must be a simple unique asset name: ' + str(name))
        require(name.lower() not in seen, 'Duplicate manifest id: ' + name)
        seen.add(name.lower())
        source = (RESEARCH / entry['file']).resolve()
        require(source.is_relative_to(RESEARCH.resolve()) and source.suffix.lower() == '.fbx',
                'Source FBX must be inside Saved/AssetResearch/Mixamo: ' + str(source))
        require(source.is_file() and source.stat().st_size > 1024,
                'Missing or empty source FBX: ' + str(source))
        digest = sha256(source)
        require(not entry.get('sha256') or entry['sha256'].lower() == digest,
                'Source hash does not match manifest: ' + name)
        entry['source_path'] = str(source)
        entry['sha256'] = digest
        entry['bytes'] = source.stat().st_size
        if 'expected_duration' in entry:
            require(math.isfinite(entry['expected_duration']) and entry['expected_duration'] > 0,
                    'Invalid expected duration: ' + name)
    return entries[0], entries[1:]


def vector(value):
    return [round(value.x, 6), round(value.y, 6), round(value.z, 6)]


def transform_data(transform):
    q = transform.rotation
    values = [*vector(transform.translation), *vector(transform.scale3d), q.x, q.y, q.z, q.w]
    require(all(math.isfinite(value) for value in values), 'Non-finite animation transform')
    return {'position': vector(transform.translation), 'scale': vector(transform.scale3d),
            'rotation_xyzw': [round(value, 7) for value in (q.x, q.y, q.z, q.w)]}


def clean_bone_name(name):
    # Resolve aliases from reported names; never rename or modify the source skeleton.
    return re.sub(r'^mixamorig\d*[:_]*', '', name.lower())


def skeleton_data(mesh):
    skeleton = mesh.get_editor_property('skeleton')
    require(skeleton is not None, 'Source mesh has no skeleton')
    pose = u.AnimPoseExtensions.get_reference_pose(skeleton)
    names = [str(name) for name in u.AnimPoseExtensions.get_bone_names(pose)]
    require(names, 'Source skeleton has no bones')
    component = u.SkeletalMeshComponent()
    component.set_skinned_asset_and_update(mesh)
    bones = [{'name': name, 'parent': str(component.get_parent_bone(name)),
              'local': transform_data(u.AnimPoseExtensions.get_bone_pose(
                  pose, name, u.AnimPoseSpaces.LOCAL)),
              'world': transform_data(u.AnimPoseExtensions.get_bone_pose(
                  pose, name, u.AnimPoseSpaces.WORLD))} for name in names]
    aliases = {}
    for role, normalized in SAMPLE_ROLES.items():
        matches = [name for name in names if clean_bone_name(name) == normalized]
        if len(matches) == 1:
            aliases[role] = matches[0]
    aliases['root'] = names[0]
    return {'mesh': mesh.get_path_name(), 'skeleton': skeleton.get_path_name(),
            'bone_count': len(bones), 'bones': bones, 'sample_bones': aliases,
            'root_is_pelvis': aliases.get('pelvis') == names[0]}


def inspect_sequence(sequence, sample_bones, respect_root_lock=False):
    length = sequence.get_editor_property('sequence_length')
    require(math.isfinite(length) and length > 0, 'Invalid animation duration')
    options = u.AnimPoseEvaluationOptions()
    options.incorporate_root_motion_into_pose = not respect_root_lock
    samples = []
    for index in range(61):
        time = length * index / 60
        pose = u.AnimPoseExtensions.get_anim_pose_at_time(sequence, time, options)
        names = {str(name).lower() for name in u.AnimPoseExtensions.get_bone_names(pose)}
        require(all(name.lower() in names for name in sample_bones.values()),
                'Animation does not contain the actual reported sample bones')
        samples.append({'time': round(time, 6), 'bones': {
            role: transform_data(u.AnimPoseExtensions.get_bone_pose(pose, name, u.AnimPoseSpaces.WORLD))
            for role, name in sample_bones.items()}})
    result = {'path': sequence.get_path_name(), 'length': length,
              'skeleton': sequence.get_editor_property('skeleton').get_path_name(),
              'enable_root_motion': sequence.get_editor_property('enable_root_motion'),
              'force_root_lock': sequence.get_editor_property('force_root_lock'),
              'root_motion_root_lock': str(sequence.get_editor_property('root_motion_root_lock')),
              'rate_scale': sequence.get_editor_property('rate_scale'),
              'additive_anim_type': str(sequence.get_editor_property('additive_anim_type')),
              'sample_bones': sample_bones, 'evaluated_with_root_lock': respect_root_lock,
              'samples': samples}
    first = samples[0]['bones']['root']['position']
    result['root_max_displacement_cm'] = max(math.dist(
        sample['bones']['root']['position'], first) for sample in samples)
    result['root_translation_bounds_cm'] = {
        axis: [min(sample['bones']['root']['position'][i] for sample in samples),
               max(sample['bones']['root']['position'][i] for sample in samples)]
        for i, axis in enumerate(('x', 'y', 'z'))}
    if {'head', 'pelvis', 'foot_l', 'foot_r'} <= sample_bones.keys():
        for role in ('head', 'pelvis'):
            values = [sample['bones'][role]['position'][2] - min(
                sample['bones'][foot]['position'][2] for foot in ('foot_l', 'foot_r'))
                for sample in samples]
            result[role + '_height_above_lowest_foot_bone_cm'] = [min(values), max(values)]
    result['height_limitation'] = 'Foot-bone origin is not the sole/floor; no cover-clearance certification.'
    return result


def preserve_fbx(entry):
    destination = PRESERVED_SOURCE / (entry['id'] + '.fbx')
    if destination.exists():
        require(sha256(destination) == entry['sha256'], 'Refusing to overwrite source FBX: ' + str(destination))
    else:
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(entry['source_path'], destination)
    require(sha256(destination) == entry['sha256'], 'Preserved FBX hash mismatch')
    return destination


def require_empty_directory(package_directory):
    disk = PROJECT / 'Content' / package_directory.removeprefix('/Game/')
    existing = (u.EditorAssetLibrary.list_assets(package_directory, recursive=True)
                if u.EditorAssetLibrary.does_directory_exist(package_directory) else [])
    require(not existing and not (disk.exists() and any(disk.iterdir())),
            'Refusing to overwrite/unrecorded import directory: ' + package_directory)


def import_fbx(entry, directory, name, skeleton=None):
    require_empty_directory(directory)
    source = preserve_fbx(entry)
    options = u.FbxImportUI()
    mesh_import = skeleton is None
    settings = {'automated_import_should_detect_type': False, 'import_as_skeletal': True,
                'mesh_type_to_import': (u.FBXImportType.FBXIT_SKELETAL_MESH if mesh_import
                                        else u.FBXImportType.FBXIT_ANIMATION),
                'import_mesh': mesh_import, 'import_animations': not mesh_import,
                'import_materials': False, 'import_textures': False,
                'create_physics_asset': False, 'override_full_name': True,
                'skeleton': skeleton}
    for key, value in settings.items():
        options.set_editor_property(key, value)
    mesh_options = options.get_editor_property('skeletal_mesh_import_data')
    for key, value in {'update_skeleton_reference_pose': False, 'use_t0_as_ref_pose': False,
                       'import_morph_targets': False, 'import_mesh_lods': False}.items():
        mesh_options.set_editor_property(key, value)
    animation_options = options.get_editor_property('anim_sequence_import_data')
    for key, value in {'animation_length': u.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME,
                       'use_default_sample_rate': False, 'custom_sample_rate': 0,
                       'import_bone_tracks': True, 'import_custom_attribute': False,
                       'add_curve_metadata_to_skeleton': False,
                       'preserve_local_transform': False}.items():
        animation_options.set_editor_property(key, value)
    for data in (mesh_options, animation_options):
        for key, value in {'convert_scene': True, 'convert_scene_unit': True,
                           'force_front_x_axis': False, 'import_uniform_scale': 1.0,
                           'import_translation': u.Vector(0, 0, 0),
                           'import_rotation': u.Rotator(0, 0, 0)}.items():
            data.set_editor_property(key, value)
    task = u.AssetImportTask()
    for key, value in {'filename': str(source), 'destination_path': directory,
                       'destination_name': name, 'automated': True, 'save': False,
                       'replace_existing': False, 'replace_existing_settings': False,
                       'factory': u.FbxFactory(), 'options': options}.items():
        task.set_editor_property(key, value)
    u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    task.get_objects()  # Wait for any importer completion before inspecting its real outputs.
    paths = u.EditorAssetLibrary.list_assets(directory, recursive=True)
    assets = [u.load_asset(path) for path in paths]
    require(assets and all(asset is not None for asset in assets), 'FBX import produced no loadable assets')
    require(all(asset.get_path_name().startswith(directory + '/') for asset in assets),
            'FBX importer placed assets outside the guarded directory')
    return assets


def save(asset):
    require(u.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False),
            'Cannot save new asset: ' + asset.get_path_name())


def main():
    assert_local_ignored()
    source, animations = read_manifest()
    report = json.loads(REPORT.read_text()) if REPORT.is_file() else {
        'schema': 1, 'provider': 'Adobe Mixamo; user-authorized local downloads',
        'redistribution': 'Local development only; FBX and derivatives excluded from public Git.',
        'source': source, 'animations': {}, 'status': 'started',
        'preserved_source_directory': str(PRESERVED_SOURCE)}
    require(report.get('schema') == 1 and report['source']['sha256'] == source['sha256'],
            'Existing inventory belongs to a different source mesh')
    previous_fbx_cvar = u.SystemLibrary.get_console_variable_int_value('Interchange.FeatureFlags.Import.FBX')
    try:
        # Use the explicitly configured FBX factory. Restore the process-local CVar afterward.
        u.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.FBX 0')
        if 'skeleton_inventory' in report:
            mesh = load(report['skeleton_inventory']['mesh'], u.SkeletalMesh)
            require(skeleton_data(mesh) == report['skeleton_inventory'],
                    'Existing source hierarchy/reference pose changed; refusing reuse')
            check_source_import_path(mesh, 'XBot')
            require(sha256(PRESERVED_SOURCE / 'XBot.fbx') == source['sha256'],
                    'Preserved base source hash changed')
        else:
            assets = import_fbx(source, MESH_DIR, 'SK_XBot')
            meshes = [asset for asset in assets if isinstance(asset, u.SkeletalMesh)]
            require(len(meshes) == 1, 'Expected exactly one X Bot skeletal mesh')
            require(all(isinstance(asset, (u.SkeletalMesh, u.Skeleton)) for asset in assets),
                    'Unexpected base-import asset type; no materials/textures/animations requested')
            mesh = meshes[0]
            report['source_import_data'] = check_source_import_path(mesh, 'XBot')
            report['skeleton_inventory'] = skeleton_data(mesh)
            for asset in assets:
                save(asset)
            # Import can return only the mesh; explicitly save its newly created skeleton too.
            save(mesh.get_editor_property('skeleton'))
            report['status'] = 'base_imported_actual_bones_reported'
            write_report(report)
        skeleton = mesh.get_editor_property('skeleton')
        bones_before = skeleton_data(mesh)
        for entry in animations:
            name = entry['id']
            previous = report['animations'].get(name)
            if previous:
                require(previous['source']['sha256'] == entry['sha256'], 'Existing clip source changed: ' + name)
                sequence = load(previous['sequence']['path'], u.AnimSequence)
                check_source_import_path(sequence, name)
                require(inspect_sequence(sequence, bones_before['sample_bones']) == previous['sequence'],
                        'Existing source clip changed; refusing reuse: ' + name)
                require(sha256(PRESERVED_SOURCE / (name + '.fbx')) == entry['sha256'],
                        'Preserved animation FBX changed: ' + name)
                continue
            assets = import_fbx(entry, ANIM_DIR + '/' + name, 'AS_Mixamo_' + name, skeleton)
            require(len(assets) == 1 and isinstance(assets[0], u.AnimSequence),
                    'Expected exactly one animation-only take: ' + name)
            sequence = assets[0]
            import_data = check_source_import_path(sequence, name)
            require(sequence.get_editor_property('skeleton') == skeleton, 'Animation source skeleton mismatch: ' + name)
            require(skeleton_data(mesh) == bones_before, 'Animation import changed the source reference skeleton')
            # Hips may be the root: never force-lock it to the T-pose height.
            sequence.set_editor_property('enable_root_motion', False)
            sequence.set_editor_property('force_root_lock', False)
            inspected = inspect_sequence(sequence, bones_before['sample_bones'])
            if 'expected_duration' in entry:
                require(abs(inspected['length'] - entry['expected_duration']) <= 1 / 30 + 0.001,
                        'Unexpected imported clip duration: ' + name)
            save(sequence)
            report['animations'][name] = {'source': entry, 'sequence': inspected, 'import_data': import_data}
            report['status'] = 'source_clips_importing'
            write_report(report)
        report['status'] = 'import_complete_not_retargeted'
        report['manifest_ids'] = [entry['id'] for entry in animations]
        write_report(report)
        u.log('GUNNER_MIXAMO_IMPORT_COMPLETE clips=' + str(len(animations)) + ' report=' + str(REPORT))
    except Exception as error:
        report['status'] = 'failed'
        report['error'] = str(error)
        write_report(report)
        raise
    finally:
        u.SystemLibrary.execute_console_command(None,
            'Interchange.FeatureFlags.Import.FBX ' + str(previous_fbx_cvar))


if __name__ == '__main__':
    main()
