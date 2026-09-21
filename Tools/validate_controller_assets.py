"""Read-only fresh-process persistence gate for the authored Xbox input assets.

Run after install_controller_input.py exits, using the GunnerEditor Python
commandlet with -NullRHI. No content is compiled, modified or saved. Only the
aggregate validation report under Saved is written; gameplay/device acceptance
is a separate rendered probe and physical-controller test.
"""
from pathlib import Path
import hashlib
import json
import math
import unreal as u

PROJECT = Path(u.Paths.project_dir()).resolve()
ROOT = '/Game/Gunner/Motion/Input'
CHARACTER = '/Game/Gunner/Motion/Characters/BP_WardenMotion'
CONFIG = ROOT + '/DA_XboxInput'
CONTEXT = ROOT + '/IMC_Xbox'
CYCLE = ROOT + '/IA_CycleWeapon'
REPORT = PROJECT / 'Saved/controller_input_asset_validation.json'
BUTTONS = {
    'traverse': 'Gamepad_FaceButton_Bottom',
    'reload': 'Gamepad_FaceButton_Left',
    'cycle_weapon': 'Gamepad_FaceButton_Top',
    'melee': 'Gamepad_FaceButton_Right',
    'crouch_toggle': 'Gamepad_RightThumbstick',
    'sprint': 'Gamepad_LeftThumbstick',
    'dodge': 'Gamepad_LeftShoulder',
    'shoulder_swap': 'Gamepad_RightShoulder',
    'rifle': 'Gamepad_DPad_Up',
    'pistol': 'Gamepad_DPad_Down',
    'jump': 'Gamepad_DPad_Right',
    'aim': 'Gamepad_LeftTrigger',
    'fire': 'Gamepad_RightTrigger',
}
AXES = {'move': 'Gamepad_Left2D', 'stick_look': 'Gamepad_Right2D'}
ACTION_FIELDS = tuple(AXES) + ('mouse_look',) + tuple(BUTTONS)


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def load(path):
    asset = u.load_asset(path)
    need(asset is not None, 'Missing asset: ' + path)
    return asset


def near(actual, expected):
    return math.isfinite(actual) and abs(actual - expected) <= 0.000001


def package_file(asset):
    package = asset.get_path_name().split('.')[0]
    need(package.startswith('/Game/'), 'Unexpected non-project package: ' + package)
    return PROJECT / ('Content' + package[len('/Game'):] + '.uasset')


def key_name(mapping):
    return str(mapping.get_editor_property('key').get_editor_property('key_name'))


def mappings(context):
    return list(context.get_editor_property('default_key_mappings').get_editor_property('mappings'))


def modifier_snapshot(modifier):
    """Compare editable values, not renamed duplicated UObject paths."""
    need(modifier is not None, 'Null input modifier')
    kind = modifier.get_class().get_name()
    result = {'class': kind}
    if kind == 'InputModifierNegate':
        result['axes'] = [bool(modifier.get_editor_property(axis)) for axis in ('x', 'y', 'z')]
    elif kind == 'InputModifierSwizzleAxis':
        result['order'] = str(modifier.get_editor_property('order'))
    elif kind == 'InputModifierDeadZone':
        result.update(lower=float(modifier.get_editor_property('lower_threshold')),
                      upper=float(modifier.get_editor_property('upper_threshold')),
                      type=str(modifier.get_editor_property('type')))
    elif kind == 'InputModifierResponseCurveExponential':
        value = modifier.get_editor_property('curve_exponent')
        result['exponent'] = [value.x, value.y, value.z]
    else:
        raise RuntimeError('Unrecognized modifier needs a complete property comparison: ' + kind)
    return result


def mapping_snapshot(mapping):
    action = mapping.get_editor_property('action')
    need(action is not None, 'Mapping without an action: ' + key_name(mapping))
    # The authored baseline contains no mapping triggers or remapping overrides.
    # Reject an unexpected extension rather than silently comparing only types.
    need(not list(mapping.get_editor_property('triggers')),
         'Unexpected mapping trigger: ' + key_name(mapping))
    need(mapping.get_editor_property('player_mappable_key_settings') is None,
         'Unexpected player-mappable override: ' + key_name(mapping))
    return {
        'key': key_name(mapping), 'action': action.get_path_name(),
        'setting_behavior': str(mapping.get_editor_property('setting_behavior')),
        'modifiers': [modifier_snapshot(value) for value in mapping.get_editor_property('modifiers')],
    }


def main():
    result = {'schema': 1, 'status': 'validating', 'actions': {},
              'limitations': ['Fresh-process saved input composition and preservation check.',
                              'Does not certify rendered behavior, Bluetooth/USB delivery or physical hardware.']}
    try:
        authored = json.loads((PROJECT / 'Saved/controller_input_authoring_report.json').read_text())
        for field, expected in (('config', CONFIG), ('context', CONTEXT), ('cycle_action', CYCLE)):
            need(authored.get(field) == expected, 'Unexpected authoring report ' + field)
        need(authored.get('buttons') == BUTTONS, 'Authoring report button layout changed')
        need(authored.get('gamepad_mapping_count') == 15, 'Authoring report gamepad count changed')
        need(near(authored.get('radial_deadzone', -1), .18)
             and near(authored.get('look_exponent', -1), 1.5)
             and near(authored.get('ads_scale', -1), .55), 'Authoring report tuning changed')
        backup = Path(authored['backup']).resolve()
        need(backup.is_relative_to(PROJECT / 'Saved/AuthoringDrafts')
             and (backup / 'BP_WardenMotion.uasset').is_file(), 'Recoverable original character backup is absent')

        old = load(ROOT + '/DA_MotionInput')
        old_context = old.get_editor_property('mapping_context')
        config = load(CONFIG)
        context = load(CONTEXT)
        cycle = load(CYCLE)
        need(isinstance(old, u.GunnerInputConfig) and isinstance(config, u.GunnerInputConfig),
             'Input Data Asset class mismatch')
        need(config.get_editor_property('mapping_context') == context, 'Xbox context assignment changed')
        need(near(config.get_editor_property('stick_aim_sensitivity_scale'), .55), 'ADS sensitivity changed')
        need(near(old.get_editor_property('stick_aim_sensitivity_scale'), 1.), 'Portable source ADS setting changed')
        need(old.get_editor_property('cycle_weapon') is None, 'Portable source acquired Xbox cycle action')
        source_files = {str(package_file(value).relative_to(PROJECT)) for value in (old, old_context)}
        expected_hashes = authored['unchanged_input_sources']
        need(set(expected_hashes) == source_files, 'Original input hash inventory is incomplete')
        for relative, expected in expected_hashes.items():
            need(hashlib.sha256((PROJECT / relative).read_bytes()).hexdigest() == expected,
                 'Original input source package changed: ' + relative)
        result['unchanged_input_sources'] = expected_hashes

        actions = {}
        for field in ACTION_FIELDS:
            action = config.get_editor_property(field)
            need(isinstance(action, u.InputAction), 'Missing InputAction: ' + field)
            if field == 'cycle_weapon':
                need(action == cycle, 'Cycle action assignment changed')
            else:
                need(action == old.get_editor_property(field), 'Existing action replaced: ' + field)
            expected_type = u.InputActionValueType.AXIS2D if field in ('move', 'mouse_look', 'stick_look') else u.InputActionValueType.BOOLEAN
            need(action.get_editor_property('value_type') == expected_type, 'Wrong action value type: ' + field)
            need(not list(action.get_editor_property('triggers')) and not list(action.get_editor_property('modifiers')),
                 'Unexpected action-level trigger/modifier affects input: ' + field)
            need(action.get_editor_property('consume_input') and not action.get_editor_property('trigger_when_paused'),
                 'Action consumption/pause behavior changed: ' + field)
            expected_accumulation = (u.InputActionAccumulationBehavior.CUMULATIVE if field == 'move'
                                     else u.InputActionAccumulationBehavior.TAKE_HIGHEST_ABSOLUTE_VALUE)
            need(action.get_editor_property('accumulation_behavior') == expected_accumulation,
                 'Action accumulation behavior changed: ' + field)
            actions[field] = action
            result['actions'][field] = {'path': action.get_path_name(), 'value_type': str(expected_type),
                                       'accumulation': str(expected_accumulation)}
        need(len({action.get_path_name() for action in actions.values()}) == len(ACTION_FIELDS),
             'Two config actions unexpectedly alias each other')

        current = mappings(context)
        keyboard = [item for item in current if not key_name(item).startswith('Gamepad_')]
        original_keyboard = [item for item in mappings(old_context) if not key_name(item).startswith('Gamepad_')]
        snapshots = [mapping_snapshot(item) for item in keyboard]
        need(snapshots == [mapping_snapshot(item) for item in original_keyboard],
             'Keyboard/mouse mapping order, actions, modifier values or settings changed')
        need(len(keyboard) == authored['keyboard_mapping_count'], 'Keyboard mapping count changed')
        result['preserved_keyboard_mapping_count'] = len(keyboard)
        result['preserved_keyboard_mappings'] = snapshots

        gamepad = [item for item in current if key_name(item).startswith('Gamepad_')]
        expected_keys = dict(AXES, **BUTTONS)
        need(len(gamepad) == 15 and len({key_name(item) for item in gamepad}) == 15,
             'Expected exactly fifteen distinct controller mappings')
        need({key_name(item) for item in gamepad} == set(expected_keys.values()),
             'Controller key inventory changed')
        mapped_by_key = {key_name(item): item for item in gamepad}
        gamepad_report = {}
        for field, key in expected_keys.items():
            item = mapped_by_key[key]
            need(item.get_editor_property('action') == actions[field], 'Wrong action for ' + key)
            summary = mapping_snapshot(item)
            modifiers = list(item.get_editor_property('modifiers'))
            expected_count = 2 if field == 'stick_look' else (1 if field == 'move' else 0)
            need(len(modifiers) == expected_count, 'Unexpected modifier count: ' + key)
            if field in AXES:
                zone = modifiers[0]
                need(isinstance(zone, u.InputModifierDeadZone)
                     and zone.get_editor_property('type') == u.DeadZoneType.RADIAL
                     and near(zone.get_editor_property('lower_threshold'), .18)
                     and near(zone.get_editor_property('upper_threshold'), 1.),
                     'Expected radial .18-to-1 dead zone: ' + key)
            if field == 'stick_look':
                curve = modifiers[1]
                need(isinstance(curve, u.InputModifierResponseCurveExponential), 'Missing stick response curve')
                exponent = curve.get_editor_property('curve_exponent')
                need(all(near(value, 1.5) for value in (exponent.x, exponent.y, exponent.z)),
                     'Stick response exponent changed')
            gamepad_report[field] = summary
        result['gamepad_mappings'] = gamepad_report
        result['gamepad_mapping_count'] = len(gamepad)
        result['ads_scale'] = config.get_editor_property('stick_aim_sensitivity_scale')

        character = load(CHARACTER)
        need(character.get_editor_property('status') == u.BlueprintStatus.BS_UP_TO_DATE,
             'Character Blueprint is not compiled cleanly')
        cdo = u.get_default_object(character.generated_class())
        need(cdo.get_editor_property('input_config') == config, 'Character does not use Xbox input config')
        mesh = cdo.get_component_by_class(u.SkeletalMeshComponent)
        graph = mesh.get_editor_property('anim_class').get_path_name()
        motion = cdo.get_editor_property('motion_settings').get_path_name()
        need(graph == authored['portable_animation_class'] and '/LicensedLocal/' not in graph,
             'Portable character animation graph changed or acquired a local dependency')
        need(motion == authored['motion_settings'] and '/LicensedLocal/' not in motion,
             'Portable character motion settings changed or acquired a local dependency')
        result.update(status='controller_input_asset_gate_passed', config=CONFIG, context=CONTEXT,
                      portable_animation_class=graph, motion_settings=motion,
                      action_count=len(actions), content_assets_modified=0)
        REPORT.write_text(json.dumps(result, indent=2) + '\n')
        u.log('GUNNER_CONTROLLER_INPUT_ASSETS_PASS mappings=15 actions=16 keyboard=' + str(len(keyboard)))
    except Exception as error:
        result.update(status='controller_input_asset_gate_failed', error=str(error))
        REPORT.write_text(json.dumps(result, indent=2) + '\n')
        raise


if __name__ == '__main__':
    main()
