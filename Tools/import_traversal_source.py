"""Preserve and import the licensed UAL2 Standard v2.1 source exactly once.

Run with UE 5.8 Editor-Cmd -run=pythonscript -script=<this file> -NullRHI.
This imports the source library, not playable actions. Only the three Slide_*
candidates are selected by retarget_traversal.py. No existing package is replaced.
"""
from pathlib import Path
import hashlib
import json
import math
import struct
import unreal as u

PROJECT = Path(u.Paths.project_dir()).resolve()
RELEASE = Path('UAL2_Standard_v2_1/source/Universal Animation Library 2[Standard]')
RESEARCH = PROJECT / 'Saved/AssetResearch/Quaternius' / RELEASE
RETAINED = PROJECT / 'ArtSource/Quaternius' / RELEASE
GLB = Path('Unreal-Godot/UAL2_Standard.glb')
SOURCE_SHA256 = '8cee20ab1bc55130092447e810e26df22dd2803eccc54f52137a7d54d7ab88a8'
DEST_PARENT = '/Game/Gunner/Animation/Source/Quaternius'
DEST = DEST_PARENT + '/UAL2_Standard'
SOURCE_MESH = DEST + '/SkeletalMeshes/UAL2_Standard'
SHORTLIST = {'Slide_Start': 25 / 30, 'Slide_Loop': 2.0, 'Slide_Exit': 0.5}
REPORT = PROJECT / 'Saved/traversal_import_inventory.json'


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def inspect_glb(data):
    require(len(data) >= 20, 'Truncated source GLB')
    magic, version, total_size = struct.unpack_from('<4sII', data)
    require(magic == b'glTF' and version == 2 and total_size == len(data),
            'Invalid GLB 2 header')
    json_size, chunk_type = struct.unpack_from('<I4s', data, 12)
    require(chunk_type == b'JSON', 'First GLB chunk is not JSON')
    source = json.loads(data[20:20 + json_size])
    animations = {animation['name']: animation for animation in source['animations']}
    require(len(animations) == len(source['animations']) == 43,
            'Unexpected source animation count or duplicate names')
    lengths = {}
    for name, expected in SHORTLIST.items():
        require(name in animations, 'Missing source candidate: ' + name)
        accessors = [source['accessors'][sampler['input']]
                     for sampler in animations[name]['samplers']]
        start = min(accessor['min'][0] for accessor in accessors)
        end = max(accessor['max'][0] for accessor in accessors)
        require(math.isfinite(start) and math.isfinite(end) and abs(start) < 0.000001,
                'Invalid source timeline: ' + name)
        lengths[name] = end - start
        require(abs(lengths[name] - expected) < 0.000001,
                'Unexpected source duration: ' + name)
    require(len(source['skins']) == 1, 'Expected one UAL2 humanoid skin')
    bones = [source['nodes'][index]['name'] for index in source['skins'][0]['joints']]
    return {'animation_names': list(animations), 'candidate_lengths': lengths,
            'source_joint_names': bones}


def load(path, cls):
    asset = u.load_asset(path)
    require(asset is not None and isinstance(asset, cls), 'Missing/wrong asset: ' + path)
    return asset


def main():
    # Check Unreal and disk state before preserving or importing anything.
    require(not u.EditorAssetLibrary.does_directory_exist(DEST),
            'Refusing to overwrite existing UAL2 source directory: ' + DEST)
    require(not (PROJECT / 'Content/Gunner/Animation/Source/Quaternius/UAL2_Standard').exists(),
            'UAL2 package directory already exists on disk; inspect it before proceeding')
    source_bytes = (RESEARCH / GLB).read_bytes()
    require(digest(source_bytes) == SOURCE_SHA256, 'Source GLB hash mismatch')
    source_inventory = inspect_glb(source_bytes)
    payloads = {GLB: source_bytes}
    for relative in (Path('License.txt'), Path('README.txt'), Path('Unreal_Setup.png')):
        payloads[relative] = (RESEARCH / relative).read_bytes()
    require(b'CC0 1.0' in payloads[Path('License.txt')], 'CC0 source license missing')
    retained_files = []
    for relative, data in payloads.items():
        path = RETAINED / relative
        require(not path.exists() or path.read_bytes() == data,
                'Refusing to replace preserved source: ' + str(path))
        retained_files.append({'file': str(path.relative_to(PROJECT)), 'sha256': digest(data)})
    # Existing identical sources can be reused; a partial Unreal import cannot.
    for relative, data in payloads.items():
        path = RETAINED / relative
        if not path.exists():
            path.parent.mkdir(parents=True, exist_ok=True)
            with path.open('xb') as output:
                output.write(data)

    task = u.AssetImportTask()
    task.filename = str(RETAINED / GLB)
    task.destination_path = DEST_PARENT
    task.automated = True
    task.save = True
    task.replace_existing = False
    u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    require(u.EditorAssetLibrary.does_directory_exist(DEST),
            'Importer did not create the expected UAL2 source package directory')
    mesh = load(SOURCE_MESH, u.SkeletalMesh)
    skeleton = mesh.get_editor_property('skeleton')
    require(skeleton is not None, 'UAL2 source mesh has no skeleton')
    reference = u.AnimPoseExtensions.get_reference_pose(skeleton)
    imported_bones = {str(name).lower() for name in u.AnimPoseExtensions.get_bone_names(reference)}
    require({name.lower() for name in source_inventory['source_joint_names']} <= imported_bones,
            'Imported skeleton lost source joints')
    inventory = []
    for path in u.EditorAssetLibrary.list_assets(DEST, recursive=True):
        asset = u.load_asset(path)
        require(asset is not None, 'Cannot load imported asset: ' + path)
        entry = {'path': path, 'type': asset.get_class().get_name()}
        if isinstance(asset, u.AnimSequence):
            require(asset.get_editor_property('skeleton') == skeleton,
                    'Source clip has a different skeleton: ' + path)
            length = asset.get_editor_property('sequence_length')
            require(math.isfinite(length) and length >= 0, 'Invalid imported duration: ' + path)
            entry.update(length=length, skeleton=skeleton.get_path_name(),
                         enable_root_motion=asset.get_editor_property('enable_root_motion'),
                         force_root_lock=asset.get_editor_property('force_root_lock'))
        inventory.append(entry)
    require(sum(entry['type'] == 'AnimSequence' for entry in inventory) == 43,
            'Importer did not produce all 43 source animations')
    for name, expected in SHORTLIST.items():
        clip = load(DEST + '/SkeletalMeshes/UAL2_Standard' + name, u.AnimSequence)
        require(abs(clip.sequence_length - expected) < 0.001,
                'Importer changed selected clip duration: ' + name)
    require(u.EditorAssetLibrary.save_directory(DEST, only_if_is_dirty=True, recursive=True),
            'Could not save the imported UAL2 directory')
    report = {'status': 'source_imported_not_gameplay_validated',
              'source_file': str((RETAINED / GLB).relative_to(PROJECT)),
              'source_sha256': SOURCE_SHA256, 'preserved_files': retained_files,
              'source_mesh': mesh.get_path_name(), 'source_skeleton': skeleton.get_path_name(),
              'raw_source': source_inventory, 'assets': inventory,
              'candidate_only': list(SHORTLIST),
              'limitation': 'Generic source slides; no dedicated cover-entry or Gears animation claim.'}
    REPORT.write_text(json.dumps(report, indent=2) + '\n')
    u.log('GUNNER_TRAVERSAL_SOURCE_IMPORTED count=43 candidates=3')


main()
