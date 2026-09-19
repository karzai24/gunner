"""Repair the generated sandbox's original DefaultSlot assignment bug, with backups.

Unreal Python returns a copy for an array's struct element: modifying that copy
requires assigning it back before writing SlotAnimTracks. This migration touches
only the eight known generated montages and refuses unexpected slot layouts.
"""
from datetime import datetime, timezone
from pathlib import Path
import json
import shutil
import unreal as u

project = Path(u.Paths.project_dir()).resolve()
root = '/Game/Gunner/Motion/Animation/Montages'
expected = {f'AM_{kind}_{action}': 'UpperBody'
            for kind in ('Rifle', 'Pistol') for action in ('Fire', 'Reload', 'Equip')}
expected.update(AM_MeleeJab='FullBody', AM_DodgeRoll='FullBody')
pending = []
for name, desired in expected.items():
    asset = u.load_asset(f'{root}/{name}')
    if not isinstance(asset, u.AnimMontage):
        raise RuntimeError('Missing generated montage: ' + name)
    tracks = list(asset.get_editor_property('slot_anim_tracks'))
    if len(tracks) != 1:
        raise RuntimeError('Refusing unexpected authored track layout: ' + name)
    current = str(tracks[0].get_editor_property('slot_name'))
    if current not in ('DefaultSlot', desired):
        raise RuntimeError(f'Refusing unexpected authored slot {name}: {current}')
    pending.append((name, desired, current, asset, tracks))

stamp = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S%fZ')
backup = project / 'Saved/AuthoringDrafts' / ('before-montage-slot-repair-' + stamp)
backup.mkdir(parents=True)
report = []
for name, desired, current, asset, tracks in pending:
    source = project / 'Content/Gunner/Motion/Animation/Montages' / (name + '.uasset')
    shutil.copy2(source, backup / source.name)
    track = tracks[0]
    track.set_editor_property('slot_name', desired)
    tracks[0] = track
    asset.set_editor_property('slot_anim_tracks', tracks)
    actual = str(asset.get_editor_property('slot_anim_tracks')[0].get_editor_property('slot_name'))
    if actual != desired:
        raise RuntimeError(f'Slot repair did not apply: {name} = {actual}')
    if not u.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError('Could not save slot repair: ' + name)
    report.append({'asset': asset.get_path_name(), 'before': current, 'after': actual})
    u.log(f'GUNNER_MONTAGE_SLOT_REPAIR {name} {current} -> {actual}')
(backup / 'manifest.json').write_text(json.dumps(report, indent=2))
(project / 'Saved/motion_montage_slot_repair.json').write_text(json.dumps(report, indent=2))
u.log(f'GUNNER_MONTAGE_SLOT_REPAIR_COMPLETE count={len(report)}')
