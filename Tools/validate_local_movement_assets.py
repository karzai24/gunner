"""Read-only fresh-process gate for the optional local movement profile."""
from pathlib import Path
import hashlib
import json
import subprocess
import unreal as u

P = Path(u.Paths.project_dir()).resolve()
LOCAL = '/Game/Gunner/LicensedLocal'

def need(condition, message):
    if not condition:
        raise RuntimeError(message)

def load(path):
    asset = u.load_asset(path)
    need(asset is not None, 'Missing asset: ' + path)
    return asset

def main():
    authored = json.loads((P / 'Saved/local_movement_authoring_report.json').read_text())
    need(authored['status'] in ('local_profile_enabled_pending_rendered_validation', 'local_profile_validated'), 'Profile not enabled')
    character = authored['character_package']
    expected = subprocess.check_output(['git', 'rev-parse', 'HEAD:' + character], cwd=P, text=True).strip()
    actual = subprocess.check_output(['git', 'hash-object', '--', character], cwd=P, text=True).strip()
    need(expected == actual, 'Public character acquired a local content dependency')
    profile = load(authored['settings'])
    graph = load(authored['graph'])
    need(profile.get_editor_property('animation_class') == graph.generated_class(), 'Profile graph mismatch')
    need(str(graph.get_editor_property('status')) == '<BlueprintStatus.BS_UP_TO_DATE: 3>', 'Graph not compiled cleanly')
    defaults = u.get_default_object(graph.generated_class())
    fields = ('directional_crouch_pose_ready', 'blind_fire_pose_ready', 'crouch_reload_pose_ready',
              'crouch_equip_pose_ready', 'crouch_dry_fire_pose_ready', 'high_cover_pose_ready', 'rifle_support_grip_pose_ready', 'crouch_transition_pose_ready')
    need(all(defaults.get_editor_property(field) for field in fields), 'An authored graph capability is absent')
    need(profile.get_editor_property('contextual_traversal') and profile.get_editor_property('directional_crouch_ready'), 'Movement capabilities disabled')
    need(profile.get_editor_property('walk_speed') == 300 and profile.get_editor_property('sprint_speed') == 500, 'Local gait tuning changed')
    retarget = json.loads((P / 'Saved/mixamo_retarget_report.json').read_text())
    need(retarget['status'] == 'retarget_complete_candidates_not_gameplay_validated', 'Retarget incomplete')
    for path, hashes in retarget['asset_package_sha256'].items():
        for file, expected_hash in hashes.items():
            need(hashlib.sha256((P / file).read_bytes()).hexdigest() == expected_hash, 'Retargeted source changed: ' + path)
    sequence_node = u.load_class(None, '/Script/AnimGraph.AnimGraphNode_SequencePlayer')
    sequences = [node.get_editor_property('node').get_editor_property('sequence').get_path_name()
                 for node in u.AnimationLibrary.get_nodes_of_class(graph, sequence_node, True)]
    need(any('A_Mixamo_CrouchedRun.' in path for path in sequences), 'Hunched rifle run absent')
    need(any('A_UAL_Sprint_Loop.' in path for path in sequences), 'Existing pistol sprint absent')
    need(not any(any(name in path for name in ('Vault', 'CoverTurn', 'CoverTransfer', 'CoverEntry', 'CoverExit', 'CoverLook')) for path in sequences), 'Unvalidated transition candidate was activated')
    blend_node = u.load_class(None, '/Script/AnimGraph.AnimGraphNode_BlendSpacePlayer')
    blends = [node.get_editor_property('node').get_editor_property('blend_space').get_path_name()
              for node in u.AnimationLibrary.get_nodes_of_class(graph, blend_node, True)]
    for name in ('BS_DirectionalCrouch.', 'BS_Crouch.', 'BS_RifleWallMovement.'):
        need(any(name in path for path in blends), 'Required distinct locomotion branch absent: ' + name)
    evaluator_node = u.load_class(None, '/Script/AnimGraph.AnimGraphNode_SequenceEvaluator')
    evaluators = [node.get_editor_property('node').get_editor_property('sequence').get_path_name()
                  for node in u.AnimationLibrary.get_nodes_of_class(graph, evaluator_node, True)]
    need(len(evaluators) == 2 and all(any(name in path for path in evaluators)
         for name in ('A_CrouchEntry.', 'A_CrouchExit.')), 'Authored crouch transitions absent')
    report = {'status': 'local_profile_asset_gate_passed', 'public_character_git_blob': actual,
              'graph': authored['graph'], 'profile': authored['settings'], 'capabilities': list(fields),
              'sequence_players': sequences, 'blend_spaces': blends, 'stance_evaluators': evaluators,
              'retargeted_packages_checked': len(retarget['asset_package_sha256']),
              'limitations': ['Static composition and source-preservation gate; rendered native probes certify behavior separately.']}
    (P / 'Saved/local_movement_asset_validation.json').write_text(json.dumps(report, indent=2) + '\n')
    u.log('GUNNER_LOCAL_MOVEMENT_ASSETS_PASS')

if __name__ == '__main__':
    main()
