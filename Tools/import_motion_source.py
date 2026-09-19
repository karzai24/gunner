"""Import the unmodified CC0 animation source, once, then inspect existing Epic clips.
Run via UnrealEditor-Cmd -run=pythonscript -script=<absolute path> -NullRHI.
"""
from pathlib import Path
import json
import unreal as u

project = Path(u.Paths.project_dir()).resolve()
dest = '/Game/Gunner/Animation/Source/Quaternius'
if u.EditorAssetLibrary.does_directory_exist(dest):
    raise RuntimeError('Source already imported; refusing to overwrite authored assets')
task = u.AssetImportTask()
task.filename = str(next((project / 'ArtSource/Quaternius').rglob('UAL1_Standard.glb')))
task.destination_path = dest
task.automated = True
task.save = True
task.replace_existing = False
u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
u.EditorAssetLibrary.save_directory(dest, only_if_is_dirty=True, recursive=True)
inventory = []
for path in u.EditorAssetLibrary.list_assets(dest, recursive=True):
    obj = u.load_asset(path)
    entry = {'path':path, 'type':obj.get_class().get_name()}
    if isinstance(obj, u.AnimSequence):
        entry.update(length=obj.sequence_length, skeleton=obj.get_editor_property('skeleton').get_path_name())
    inventory.append(entry)
(project / 'Saved/motion_import_inventory.json').write_text(json.dumps(inventory, indent=2))
for path in ['/Game/Characters/Mannequins/Anims/Rifle/MM_Rifle_Fire', '/Game/Characters/Mannequins/Anims/Rifle/MM_Rifle_Reload', '/Game/Characters/Mannequins/Anims/Pistol/MM_Pistol_Fire', '/Game/Characters/Mannequins/Anims/Pistol/MM_Pistol_Reload']:
    a = u.load_asset(path)
    u.log(f'GUNNER_CLIP {path} length={a.sequence_length} additive={a.get_editor_property("additive_anim_type")} ref={a.get_editor_property("ref_pose_type")}')
u.log(f'GUNNER_MOTION_SOURCE_IMPORTED count={len(inventory)}')
