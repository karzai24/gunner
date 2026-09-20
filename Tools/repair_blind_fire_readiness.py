"""One-time readiness repair for the known generated arm-IK animation Blueprint.

Native validation checks the two non-stretching IK chains, hand rotations, arm
masks and action slots before modifying its class default. The existing package
is backed up. A character still on the known original graph is assigned the new
graph after its own backup; unknown animation assignments are never replaced.
"""
from pathlib import Path
import datetime
import hashlib
import json
import shutil
import unreal as u

project = Path(u.Paths.project_dir()).resolve()
path = '/Game/Gunner/Motion/Animation/ABP_WardenBlindFire'
graph = u.load_asset(path)
if not graph or not isinstance(graph, u.AnimBlueprint) or not graph.generated_class():
    raise RuntimeError('Expected generated blind-fire animation Blueprint is missing')
defaults = u.get_default_object(graph.generated_class())
was_ready = defaults.get_editor_property('blind_fire_pose_ready')
character = u.load_asset('/Game/Gunner/Motion/Characters/BP_WardenMotion')
if not character or not character.generated_class():
    raise RuntimeError('Expected motion character Blueprint is missing')
character_defaults = u.get_default_object(character.generated_class())
mesh = character_defaults.get_component_by_class(u.SkeletalMeshComponent)
assigned_class = mesh.get_editor_property('anim_class')
assigned_path = assigned_class.get_path_name() if assigned_class else None
old_path = '/Game/Gunner/Motion/Animation/ABP_WardenMotion.ABP_WardenMotion_C'
new_path = graph.generated_class().get_path_name()
u.log('GUNNER_BLIND_FIRE_REPAIR_INITIAL ' + json.dumps({
    'new_graph_ready': bool(was_ready), 'assigned_class': assigned_path,
    'expected_class': new_path}))
if assigned_path not in (old_path, new_path):
    raise RuntimeError('Unexpected assigned animation class; refusing repair: ' + str(assigned_path))
needs_assignment = assigned_path == old_path
if was_ready and not needs_assignment:
    raise RuntimeError('Readiness and assignment already repaired; refusing repeat repair')
for kind in ('Rifle', 'Pistol'):
    weapon = u.load_asset('/Game/Gunner/Motion/Weapons/DA_' + kind)
    u.log('GUNNER_BLIND_FIRE_GRIP_INITIAL ' + json.dumps({'weapon': kind,
        'ready': bool(weapon and weapon.get_editor_property('blind_fire_grip_ready'))}))
    if not weapon or not weapon.get_editor_property('blind_fire_grip_ready'):
        raise RuntimeError('Derived grip data did not persist; repair that separately: ' + kind)

source = project / 'Content/Gunner/Motion/Animation/ABP_WardenBlindFire.uasset'
backup = project / 'Saved/AuthoringDrafts' / (
    'before-blind-readiness-' + datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
backup.mkdir(parents=True, exist_ok=False)
shutil.copy2(source, backup / source.name)
if needs_assignment:
    character_source = project / 'Content/Gunner/Motion/Characters/BP_WardenMotion.uasset'
    shutil.copy2(character_source, backup / character_source.name)
report = {'asset': path, 'prior_ready': bool(was_ready), 'backup': str(backup),
          'assigned_class_before': assigned_path, 'assignment_repaired': needs_assignment,
          'prior_sha256': hashlib.sha256(source.read_bytes()).hexdigest()}
(backup / 'manifest.json').write_text(json.dumps(report, indent=2))
if not was_ready:
    if not u.GunnerAnimationBuilder.repair_blind_fire_readiness(graph):
        raise RuntimeError('Native graph validation refused readiness repair')
    defaults.modify()
    graph.modify()
    if not u.EditorAssetLibrary.save_loaded_asset(graph, only_if_is_dirty=False):
        raise RuntimeError('Could not persist the repaired animation Blueprint')
if not defaults.get_editor_property('blind_fire_pose_ready'):
    raise RuntimeError('Readiness did not remain enabled after save')
if needs_assignment:
    # Compile before updating the CDO: compilation may replace inherited component
    # templates and discard an assignment made to the prior generated defaults.
    u.BlueprintEditorLibrary.compile_blueprint(character)
    character_defaults = u.get_default_object(character.generated_class())
    mesh = character_defaults.get_component_by_class(u.SkeletalMeshComponent)
    mesh.modify()
    character_defaults.modify()
    character.modify()
    mesh.set_anim_instance_class(graph.generated_class())
    if not u.EditorAssetLibrary.save_loaded_asset(character, only_if_is_dirty=False):
        raise RuntimeError('Could not persist repaired character assignment')
updated = u.get_default_object(character.generated_class()).get_component_by_class(u.SkeletalMeshComponent)
report['assigned_class_after'] = updated.get_editor_property('anim_class').get_path_name()
if report['assigned_class_after'] != new_path:
    raise RuntimeError('Character did not retain the expected new animation class')
report['ready_after_save'] = True
report['saved_sha256'] = hashlib.sha256(source.read_bytes()).hexdigest()
if not was_ready and report['saved_sha256'] == report['prior_sha256']:
    raise RuntimeError('Package bytes did not change despite the repaired class default')
(project / 'Saved/blind_fire_readiness_repair.json').write_text(json.dumps(report, indent=2))
u.log('GUNNER_BLIND_FIRE_READINESS_REPAIRED ' + json.dumps(report))
