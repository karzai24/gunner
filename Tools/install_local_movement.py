"""Compose locally retargeted armed high-cover movement and rifle sprint.

Creates an ignored profile/graph and preserves the current character for recovery.
Enable it after closing Unreal with Tools/enable_local_movement.py.
Requires Mixamo import/retarget reports and the installed traversal asset gate.
No source clip, existing graph or motion settings are overwritten.
"""
from pathlib import Path
import datetime
import hashlib
import json
import shutil
import unreal as u

P = Path(u.Paths.project_dir()).resolve()
M = '/Game/Gunner/Motion'
A = '/Game/Characters/Mannequins/Anims'
LOCAL = '/Game/Gunner/LicensedLocal'
GRAPH = LOCAL + '/Mixamo/Runtime/ABP_WardenCoverMovement'
BLEND = LOCAL + '/Mixamo/Runtime/BS_RifleWallMovement'
SETTINGS = LOCAL + '/Mixamo/Runtime/DA_CoverMovement'
CHARACTER = M + '/Characters/BP_WardenMotion'
REPORT = P / 'Saved/local_movement_authoring_report.json'


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def load(path):
    asset = u.load_asset(path)
    need(asset is not None, 'Missing asset: ' + path)
    return asset


def main():
    for output in (GRAPH, BLEND, SETTINGS):
        need(not u.EditorAssetLibrary.does_asset_exist(output), 'Refusing existing asset: ' + output)
    retarget = json.loads((P / 'Saved/mixamo_retarget_report.json').read_text())
    need(retarget['status'] == 'retarget_complete_candidates_not_gameplay_validated', 'Retarget not complete')
    need(json.loads((P / 'Saved/installed_traversal_asset_validation.json').read_text())['status'] ==
         'persisted_asset_gate_passed', 'Directional asset gate has not passed')
    sprint_path = LOCAL + '/Mixamo/Retargeted/Manny/A_Mixamo_CrouchedRun'
    for path in (sprint_path, *[LOCAL + '/Mixamo/Retargeted/Manny/A_Mixamo_' + name for name in ('CoverIdleRifle', 'CoverLeftRifle', 'CoverRightRifle')]):
        for file, expected in retarget['asset_package_sha256'][path].items():
            need(hashlib.sha256((P / file).read_bytes()).hexdigest() == expected, 'Candidate package changed')
    character = load(CHARACTER)
    cdo = u.get_default_object(character.generated_class())
    component = cdo.get_component_by_class(u.SkeletalMeshComponent)
    prior_class = component.get_editor_property('anim_class').get_path_name()
    need(prior_class in (LOCAL + '/MoverTraversal/ABP_WardenTraversal.ABP_WardenTraversal_C',
                         LOCAL + '/Mixamo/Runtime/ABP_WardenMixamoSprint.ABP_WardenMixamoSprint_C',
                         LOCAL + '/Mixamo/Runtime/ABP_WardenMixamoCover.ABP_WardenMixamoCover_C',
                         M + '/Animation/ABP_WardenMotionPolish.ABP_WardenMotionPolish_C'),
         'Unexpected current graph; refuse local replacement')
    backup = P / 'Saved/AuthoringDrafts' / ('before-local-movement-' + datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
    backup.mkdir(parents=True, exist_ok=False)
    source = P / 'Content/Gunner/Motion/Characters/BP_WardenMotion.uasset'
    shutil.copy2(source, backup / source.name)
    mesh = load('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
    high_cover = u.GunnerAnimationBuilder.create_directional_blend_space(
        BLEND, mesh.get_editor_property('skeleton'), mesh,
        [load(LOCAL + '/Mixamo/Retargeted/Manny/A_Mixamo_' + name) for name in
         ('CoverIdleRifle', 'CoverLeftRifle', 'CoverRightRifle')],
        [u.Vector(0, 0, 0), u.Vector(0, -110, 0), u.Vector(0, 110, 0)], 220.0)
    need(high_cover is not None, 'High-cover Blend Space failed')
    graph = u.GunnerAnimationBuilder.create_locomotion_blueprint(
        GRAPH, mesh.get_editor_property('skeleton'), mesh,
        load(M + '/Animation/BS_Rifle'), load(M + '/Animation/BS_Pistol'),
        load(A + '/Rifle/Jump/MM_Rifle_Jump_Fall_Loop'), load(A + '/Pistol/Jump/MM_Pistol_Jump_Fall_Loop'),
        load(LOCAL + '/MoverTraversal/BS_DirectionalCrouch'), load(sprint_path),
        load(A + '/Rifle/AIM/AO_Rifle'), load(A + '/Pistol/Aim/AO_Pistol'),
        True, True, True, True, protective_cover_locomotion=load(M + '/Animation/BS_Crouch'),
        high_cover_locomotion=high_cover,
        pistol_sprint=load('/Game/Gunner/Animation/Retargeted/Manny/A_UAL_Sprint_Loop'),
        include_rifle_support_grip=True,
        crouch_entry=load(LOCAL + '/MoverTraversal/A_CrouchEntry'),
        crouch_exit=load(LOCAL + '/MoverTraversal/A_CrouchExit'))
    need(graph is not None, 'Graph creation failed')
    need(str(graph.get_editor_property('status')) == '<BlueprintStatus.BS_UP_TO_DATE: 3>', 'Graph did not compile cleanly')
    motion = u.EditorAssetLibrary.duplicate_asset(cdo.get_editor_property('motion_settings').get_path_name(), SETTINGS)
    need(motion is not None, 'Cannot create local movement tuning')
    motion.set_editor_property('animation_class', graph.generated_class())
    motion.set_editor_property('directional_crouch_ready', True)
    motion.set_editor_property('contextual_traversal', True)
    motion.set_editor_property('walk_speed', 300.0)
    motion.set_editor_property('sprint_speed', 500.0)
    need(u.EditorAssetLibrary.save_loaded_asset(motion, only_if_is_dirty=False), 'Cannot save movement tuning')
    REPORT.write_text(json.dumps({'status': 'local_profile_created_pending_enable_and_validation',
        'graph': GRAPH, 'settings': SETTINGS, 'cover_blend': BLEND, 'sprint': sprint_path, 'prior_class': prior_class,
        'backup_character': str(backup / source.name),
        'character_package': str(source.relative_to(P)),
        'character_sha256_before_enable': hashlib.sha256(source.read_bytes()).hexdigest(),
        'profile_package_sha256': hashlib.sha256((P / 'Content' / (SETTINGS.removeprefix('/Game/') + '.uasset')).read_bytes()).hexdigest(),
        'limitations': ['Rifle run candidate needs rendered grip, foot and transition acceptance.',
                        'Rifle standing high-cover idle and lateral gait need live acceptance; pistol retains existing pose/sprint.',
                        'No corner, cover slide or vault gameplay is enabled by this graph.',
                        'Use Tools/enable_local_movement.py after closing Unreal; it restores the portable Blueprint and enables an ignored local config profile.']}, indent=2) + '\n')
    u.log('GUNNER_LOCAL_MOVEMENT_PROFILE_CREATED')


if __name__ == '__main__':
    main()
