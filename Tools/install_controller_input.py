"""One-time Xbox input composition; preserve original contexts and character backup.

Run with the built GunnerEditor Python commandlet. Existing outputs are never
replaced. Input only: no animation or licensed-local dependency is introduced.
"""
from pathlib import Path
import datetime
import hashlib
import json
import shutil
import unreal as u

ROOT = '/Game/Gunner/Motion/Input'
CHARACTER = '/Game/Gunner/Motion/Characters/BP_WardenMotion'
OLD_CONFIG = ROOT + '/DA_MotionInput'
CONFIG = ROOT + '/DA_XboxInput'
CONTEXT = ROOT + '/IMC_Xbox'
CYCLE = ROOT + '/IA_CycleWeapon'
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


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def load(path):
    value = u.load_asset(path)
    need(value is not None, 'Missing asset: ' + path)
    return value


def save(asset):
    asset.modify()
    need(u.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False),
         'Could not save: ' + asset.get_path_name())
    return asset


def main():
    p = Path(u.Paths.project_dir()).resolve()
    lib = u.EditorAssetLibrary
    need(not any(lib.does_asset_exist(path) for path in (CONFIG, CONTEXT, CYCLE)),
         'Xbox outputs already exist; refusing to overwrite authored content')
    character = load(CHARACTER)
    cdo = u.get_default_object(character.generated_class())
    old = load(OLD_CONFIG)
    need(cdo.get_editor_property('input_config') == old, 'Unexpected character input config')
    original_context = old.get_editor_property('mapping_context')
    original_paths = [p / ('Content' + path.split('/Game', 1)[1] + '.uasset')
                      for path in [OLD_CONFIG, original_context.get_path_name().split('.')[0]]]
    original_hashes = {str(path.relative_to(p)): hashlib.sha256(path.read_bytes()).hexdigest()
                       for path in original_paths}
    backup = p / 'Saved/AuthoringDrafts' / ('before-controller-' + datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
    backup.mkdir(parents=True, exist_ok=False)
    char_file = p / 'Content/Gunner/Motion/Characters/BP_WardenMotion.uasset'
    shutil.copy2(char_file, backup / char_file.name)
    old_graph = cdo.get_component_by_class(u.SkeletalMeshComponent).get_editor_property('anim_class').get_path_name()
    old_motion = cdo.get_editor_property('motion_settings').get_path_name()
    context = lib.duplicate_asset(original_context.get_path_name(), CONTEXT)
    config = lib.duplicate_asset(OLD_CONFIG, CONFIG)
    need(context is not None and config is not None, 'Input copies failed')
    factory = u.DataAssetFactory()
    factory.set_editor_property('data_asset_class', u.InputAction)
    cycle = u.AssetToolsHelpers.get_asset_tools().create_asset('IA_CycleWeapon', ROOT, u.InputAction, factory)
    need(cycle is not None, 'Cycle action creation failed')
    cycle.set_editor_property('value_type', u.InputActionValueType.BOOLEAN)
    save(cycle)
    config.set_editor_property('cycle_weapon', cycle)
    config.set_editor_property('mapping_context', context)
    config.set_editor_property('stick_aim_sensitivity_scale', .55)
    source_mappings = list(context.get_editor_property('default_key_mappings').get_editor_property('mappings'))
    mappings = [m for m in source_mappings
                if not str(m.get_editor_property('key').get_editor_property('key_name')).startswith('Gamepad_')]
    keyboard_count = len(mappings)

    def bind(field, key, stick=False, response=False):
        action = config.get_editor_property(field)
        need(action is not None, 'Missing action: ' + field)
        item = u.EnhancedActionKeyMapping()
        item.set_editor_property('action', action)
        value = u.Key()
        value.set_editor_property('key_name', key)
        item.set_editor_property('key', value)
        modifiers = []
        if stick:
            deadzone = u.new_object(u.InputModifierDeadZone, outer=context)
            deadzone.set_editor_property('lower_threshold', .18)
            deadzone.set_editor_property('upper_threshold', 1.0)
            deadzone.set_editor_property('type', u.DeadZoneType.RADIAL)
            modifiers.append(deadzone)
        if response:
            curve = u.new_object(u.InputModifierResponseCurveExponential, outer=context)
            curve.set_editor_property('curve_exponent', u.Vector(1.5, 1.5, 1.5))
            modifiers.append(curve)
        item.set_editor_property('modifiers', modifiers)
        mappings.append(item)

    bind('move', 'Gamepad_Left2D', stick=True)
    bind('stick_look', 'Gamepad_Right2D', stick=True, response=True)
    for field, key in BUTTONS.items():
        bind(field, key)
    context.set_editor_property('default_key_mappings', u.InputMappingContextMappingData(mappings=mappings))
    save(context)
    save(config)
    cdo.modify(); character.modify()
    cdo.set_editor_property('input_config', config)
    u.BlueprintEditorLibrary.compile_blueprint(character)
    save(character)
    cdo = u.get_default_object(character.generated_class())
    need(cdo.get_editor_property('input_config') == config, 'Controller config assignment failed')
    need(cdo.get_component_by_class(u.SkeletalMeshComponent).get_editor_property('anim_class').get_path_name() == old_graph,
         'Character animation graph changed')
    need(cdo.get_editor_property('motion_settings').get_path_name() == old_motion, 'Motion settings changed')
    for relative, expected in original_hashes.items():
        need(hashlib.sha256((p / relative).read_bytes()).hexdigest() == expected, 'Original input asset changed')
    report = {'status': 'authored_pending_rendered_validation', 'config': CONFIG, 'context': CONTEXT,
              'cycle_action': CYCLE, 'buttons': BUTTONS, 'backup': str(backup),
              'keyboard_mapping_count': keyboard_count, 'gamepad_mapping_count': len(BUTTONS) + 2,
              'radial_deadzone': .18, 'look_exponent': 1.5, 'ads_scale': .55,
              'unchanged_input_sources': original_hashes, 'portable_animation_class': old_graph,
              'motion_settings': old_motion}
    (p / 'Saved/controller_input_authoring_report.json').write_text(json.dumps(report, indent=2) + '\n')
    u.log('GUNNER_CONTROLLER_INPUT_INSTALLED')


if __name__ == '__main__':
    main()
