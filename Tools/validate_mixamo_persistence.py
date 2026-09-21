"""Read-only Mixamo save/reload gate; run in a fresh Unreal editor commandlet.

UE 5.8 Editor-Cmd: -run=pythonscript -script=<this file> -NullRHI.
Run after import/retarget have exited, and again after a normal-editor session.
Compares all 61 saved sample times for every source, raw Manny derivative and
root-locked Manny derivative with the authoring reports. Also verifies original
FBX hashes, external source import paths, empty derivative reimport metadata,
reference skeletons, root policies and saved derivative/rig package hashes.
Never imports, retargets, compiles, edits or saves a content asset. Only its JSON
report in Saved is written. This is persistence proof, not gameplay acceptance.
"""
from pathlib import Path
import importlib.util
import json
import math
import unreal as u

PROJECT = Path(u.Paths.project_dir()).resolve()
spec = importlib.util.spec_from_file_location(
    'gunner_mixamo_persistence_retarget_helpers', PROJECT / 'Tools/retarget_mixamo.py')
r = importlib.util.module_from_spec(spec)
spec.loader.exec_module(r)
h = r.h
require = h.require
REPORT = PROJECT / 'Saved/mixamo_persistence_validation.json'
POSITION_TOLERANCE_CM = 0.001
SCALE_TOLERANCE = 0.00001
ROTATION_TOLERANCE_DEGREES = 0.01
SAMPLE_COUNT = 61


def quaternion_angle_degrees(left, right):
    require(len(left) == len(right) == 4, 'Invalid sampled quaternion')
    denominator = math.sqrt(sum(value * value for value in left) *
                            sum(value * value for value in right))
    require(denominator > 0 and math.isfinite(denominator), 'Degenerate sampled quaternion')
    dot = abs(sum(a * b for a, b in zip(left, right))) / denominator
    return math.degrees(2 * math.acos(min(1.0, dot)))


def compare_pose_report(actual, expected, label):
    for field in ('path', 'skeleton', 'enable_root_motion', 'force_root_lock',
                  'root_motion_root_lock', 'additive_anim_type', 'sample_bones',
                  'evaluated_with_root_lock'):
        require(actual[field] == expected[field], label + ': property changed: ' + field)
    for field in ('length', 'rate_scale'):
        require(abs(actual[field] - expected[field]) <= 0.000001,
                label + ': property changed: ' + field)
    require(len(actual['samples']) == len(expected['samples']) == SAMPLE_COUNT,
            label + ': expected all 61 sample times')
    maxima = {'position_error_cm': 0.0, 'scale_error': 0.0, 'rotation_error_degrees': 0.0}
    compared_bones = 0
    for index, (current, recorded) in enumerate(zip(actual['samples'], expected['samples'])):
        require(abs(current['time'] - recorded['time']) <= 0.000001,
                label + ': sample time changed at ' + str(index))
        require(set(current['bones']) == set(recorded['bones']) == set(expected['sample_bones']),
                label + ': sampled bone inventory changed')
        for bone, transform in current['bones'].items():
            previous = recorded['bones'][bone]
            position = math.dist(transform['position'], previous['position'])
            scale = max(abs(a - b) for a, b in zip(transform['scale'], previous['scale']))
            rotation = quaternion_angle_degrees(transform['rotation_xyzw'], previous['rotation_xyzw'])
            require(all(math.isfinite(value) for value in (position, scale, rotation)),
                    label + ': non-finite comparison')
            maxima['position_error_cm'] = max(maxima['position_error_cm'], position)
            maxima['scale_error'] = max(maxima['scale_error'], scale)
            maxima['rotation_error_degrees'] = max(maxima['rotation_error_degrees'], rotation)
            require(position <= POSITION_TOLERANCE_CM and scale <= SCALE_TOLERANCE and
                    rotation <= ROTATION_TOLERANCE_DEGREES,
                    '{}: pose changed at sample {} time={} bone={} position_cm={} scale={} rotation_deg={}'
                    .format(label, index, current['time'], bone, position, scale, rotation))
            compared_bones += 1
    return dict(maxima, samples=SAMPLE_COUNT, bone_transforms=compared_bones)


def check_tracks(sequence, reference_names, label):
    model = sequence.get_editor_property('data_model_interface')
    require(model is not None, label + ': editable data model missing')
    # Unreal bone identifiers are FNames; their matching is case-insensitive.
    names = [str(name).lower() for name in model.get_bone_track_names()]
    require(names and len(names) == model.get_num_bone_tracks() and len(set(names)) == len(names),
            label + ': invalid editable bone track inventory')
    reference_names = {name.lower() for name in reference_names}
    require(set(names) <= reference_names, label + ': editable tracks refer to another skeleton: ' + str(sorted(set(names)-reference_names)))
    return {'bone_tracks': len(names), 'keys': model.get_number_of_keys(),
            'frames': model.get_number_of_frames()}


def main():
    result = {'schema': 1, 'status': 'validating', 'clips': {}, 'source_fbx': {},
              'tolerances': {'position_cm': POSITION_TOLERANCE_CM, 'scale': SCALE_TOLERANCE,
                             'rotation_degrees': ROTATION_TOLERANCE_DEGREES},
              'evaluation': 'Raw editable pose, all 61 report times, source and target raw/root-locked.',
              'limitations': ['Run in a separate process after authoring exits.',
                              'Does not certify compressed runtime poses or rendered gameplay.']}
    try:
        h.assert_local_ignored()
        imported = json.loads(h.REPORT.read_text())
        retargeted = json.loads(r.REPORT.read_text())
        require(imported.get('schema') == 1 and imported.get('status') == 'import_complete_not_retargeted',
                'Source import inventory is incomplete')
        require(retargeted.get('schema') == 1 and retargeted.get('status') ==
                'retarget_complete_candidates_not_gameplay_validated', 'Retarget report is incomplete')
        require(imported.get('preserved_source_directory') == str(h.PRESERVED_SOURCE),
                'Import inventory does not use the external preserved-source policy')
        source, animations = h.read_manifest()  # Rechecks every original download against manifest SHA.
        ids = {entry['id'] for entry in animations}
        require(ids and ids == set(imported['animations']) == set(imported['manifest_ids']) ==
                set(retargeted['source_clips']) == set(retargeted['target_clips']),
                'Manifest/import/retarget clip inventory differs')
        require(source['sha256'] == imported['source']['sha256'] == retargeted['source_mesh_sha256'],
                'Original mesh FBX differs from authoring provenance')
        result['baseline_report_sha256'] = {str(path.relative_to(PROJECT)): h.sha256(path)
                                            for path in (h.MANIFEST, h.REPORT, r.REPORT)}
        for entry in [source, *animations]:
            name = entry['id']
            preserved = (h.PRESERVED_SOURCE / (name + '.fbx')).resolve()
            require(preserved.is_relative_to(h.PRESERVED_SOURCE.resolve()) and
                    not preserved.is_relative_to((PROJECT / 'Content').resolve()),
                    'Preserved source resolves inside watched Content: ' + name)
            require(preserved.is_file() and h.sha256(preserved) == entry['sha256'],
                    'Preserved FBX differs from original download: ' + name)
            if name != 'XBot':
                require(imported['animations'][name]['source']['sha256'] == entry['sha256'],
                        'Original clip differs from authoring provenance: ' + name)
            result['source_fbx'][name] = {'sha256': entry['sha256'],
                                        'original': entry['source_path'], 'preserved': str(preserved)}

        source_mesh = h.load(imported['skeleton_inventory']['mesh'], u.SkeletalMesh)
        target_mesh = h.load(r.TARGET_MESH, u.SkeletalMesh)
        require(h.skeleton_data(source_mesh) == imported['skeleton_inventory'],
                'Persisted source reference skeleton changed')
        require(h.skeleton_data(target_mesh) == retargeted['target_reference_inventory'],
                'Persisted canonical target reference skeleton changed')
        require(retargeted['source_mesh'] == source_mesh.get_path_name() and
                retargeted['target_mesh'] == r.TARGET_MESH, 'Reported retarget meshes changed')
        require(h.check_source_import_path(source_mesh, 'XBot') == imported['source_import_data'],
                'Source mesh import data class/path changed')
        source_skeleton = source_mesh.get_editor_property('skeleton')
        target_skeleton = target_mesh.get_editor_property('skeleton')
        require(source_skeleton != target_skeleton, 'Source and Manny must retain distinct skeletons')
        source_names = {bone['name'] for bone in imported['skeleton_inventory']['bones']}
        target_names = {bone['name'] for bone in retargeted['target_reference_inventory']['bones']}
        source_bones = imported['skeleton_inventory']['sample_bones']
        expected_packages = {r.RIGS + '/' + name for name in (r.SOURCE_RIG, r.TARGET_RIG, r.RETARGETER)}
        expected_packages.update(r.OUTPUT + '/A_Mixamo_' + name for name in ids)
        require(set(retargeted['asset_package_sha256']) == expected_packages,
                'Retarget package hash inventory is incomplete')
        actual_packages = {path.split('.', 1)[0] for directory in (r.RIGS, r.OUTPUT)
                           for path in u.EditorAssetLibrary.list_assets(directory, recursive=True)}
        require(actual_packages == expected_packages, 'Retarget directories contain missing/unrecorded packages')
        for path in sorted(expected_packages):
            require(r.package_hashes(path) == retargeted['asset_package_sha256'][path],
                    'Persisted retarget package changed since authoring: ' + path)
        result['verified_package_count'] = len(expected_packages)

        for name in sorted(ids):
            original = imported['animations'][name]
            source_clip = h.load(original['sequence']['path'], u.AnimSequence)
            target_clip = h.load(r.OUTPUT + '/A_Mixamo_' + name, u.AnimSequence)
            require(source_clip.get_editor_property('skeleton') == source_skeleton and
                    target_clip.get_editor_property('skeleton') == target_skeleton,
                    'Persisted clip skeleton mismatch: ' + name)
            require(h.check_source_import_path(source_clip, name) == original['import_data'],
                    'Source clip import data class/path changed: ' + name)
            r.require_no_reimport_source(target_clip)
            require(not source_clip.get_editor_property('enable_root_motion') and
                    not source_clip.get_editor_property('force_root_lock'),
                    'Source Hips/root lock must remain disabled: ' + name)
            require(not target_clip.get_editor_property('enable_root_motion') and
                    target_clip.get_editor_property('force_root_lock') and
                    target_clip.get_editor_property('root_motion_root_lock') == u.RootMotionRootLock.REF_POSE,
                    'Manny dedicated-root policy changed: ' + name)
            require(original['sequence'] == retargeted['source_clips'][name],
                    'Import and retarget baselines disagree: ' + name)
            source_pose = h.inspect_sequence(source_clip, source_bones)
            raw_pose = h.inspect_sequence(target_clip, r.TARGET_BONES)
            locked_pose = h.inspect_sequence(target_clip, r.TARGET_BONES, respect_root_lock=True)
            require(abs(source_pose['length'] - raw_pose['length']) < 0.001,
                    'Retarget duration differs from source: ' + name)
            clip_result = {
                'source': compare_pose_report(source_pose, original['sequence'], name + '/source'),
                'target_raw': compare_pose_report(raw_pose, retargeted['target_clips'][name]['raw'], name + '/raw'),
                'target_in_place': compare_pose_report(locked_pose, retargeted['target_clips'][name]['in_place'],
                                                      name + '/in_place'),
                'source_model': check_tracks(source_clip, source_names, name + '/source'),
                'target_model': check_tracks(target_clip, target_names, name + '/target'),
                'root_validation': r.validate_lock(raw_pose, locked_pose),
                'target_import_data': {'class': 'AssetImportData', 'filenames': []}}
            result['clips'][name] = clip_result
        # Confirm evaluation did not save/mutate package files during this gate.
        for path in sorted(expected_packages):
            require(r.package_hashes(path) == retargeted['asset_package_sha256'][path],
                    'Package changed while validating: ' + path)
        result.update(status='persisted_pose_gate_passed', clip_count=len(ids),
                      pose_samples_compared=len(ids) * SAMPLE_COUNT * 3)
        u.log('GUNNER_MIXAMO_PERSISTENCE_PASS clips={} pose_samples={} metadata_empty=true source_fbx_external=true'
              .format(len(ids), result['pose_samples_compared']))
    except Exception as error:
        result.update(status='failed', error=str(error))
        raise
    finally:
        REPORT.write_text(json.dumps(result, indent=2) + '\n')


if __name__ == '__main__':
    main()
