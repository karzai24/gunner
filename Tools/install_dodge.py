"""One-time upgrade for the first motion-range assets. Fresh composition already includes dodge.
Refuses an existing roll; backs up every existing asset it changes before loading it.
"""
from pathlib import Path
import shutil
import unreal as u

root='/Game/Gunner/Motion'
lib=u.EditorAssetLibrary
tools=u.AssetToolsHelpers.get_asset_tools()
roll_path=f'{root}/Animation/Montages/AM_DodgeRoll'
if lib.does_asset_exist(roll_path):
    raise RuntimeError('Dodge is already authored; refusing to overwrite it')
project=Path(u.Paths.project_dir()).resolve()
backup=project/'Saved/AuthoringDrafts/before-dodge'
for relative in ['Input/DA_MotionInput','Input/IMC_Motion','Characters/BP_WardenMotion']:
    src=project/f'Content/Gunner/Motion/{relative}.uasset'
    dst=backup/f'{relative}.uasset'
    if dst.exists():
        if src.read_bytes()!=dst.read_bytes():raise RuntimeError(f'Asset changed since backup: {src}')
        continue
    dst.parent.mkdir(parents=True,exist_ok=True)
    shutil.copy2(src,dst)
factory=u.AnimMontageFactory()
factory.target_skeleton=u.load_asset('/Game/Characters/Mannequins/Meshes/SK_Mannequin')
factory.source_animation=u.load_asset('/Game/Gunner/Animation/Retargeted/Manny/A_UAL_Roll')
roll=tools.create_asset('AM_DodgeRoll',f'{root}/Animation/Montages',u.AnimMontage,factory)
tracks=roll.get_editor_property('slot_anim_tracks')
track=tracks[0]
track.set_editor_property('slot_name','FullBody')
tracks[0]=track
roll.set_editor_property('slot_anim_tracks',tracks)
if str(roll.get_editor_property('slot_anim_tracks')[0].get_editor_property('slot_name'))!='FullBody':
    raise RuntimeError('Roll montage slot assignment failed')
roll.set_editor_property('rate_scale',1.65)
for prop,seconds in [('blend_in',0.07),('blend_out',0.12)]:
    blend=roll.get_editor_property(prop)
    blend.set_editor_property('blend_time',seconds)
    roll.set_editor_property(prop,blend)
factory=u.DataAssetFactory()
factory.set_editor_property('data_asset_class',u.InputAction)
action=tools.create_asset('IA_Dodge',f'{root}/Input',u.InputAction,factory)
context=u.load_asset(f'{root}/Input/IMC_Motion')
data=context.get_editor_property('default_key_mappings')
mappings=list(data.get_editor_property('mappings'))
for key in ['E','Gamepad_LeftShoulder']:
    k=u.Key();k.set_editor_property('key_name',key)
    entry=u.EnhancedActionKeyMapping()
    entry.set_editor_property('action',action)
    entry.set_editor_property('key',k)
    mappings.append(entry)
context.set_editor_property('default_key_mappings',u.InputMappingContextMappingData(mappings=mappings))
config=u.load_asset(f'{root}/Input/DA_MotionInput')
config.set_editor_property('dodge',action)
character=u.load_asset(f'{root}/Characters/BP_WardenMotion')
u.BlueprintEditorLibrary.compile_blueprint(character)
cdo=u.get_default_object(character.generated_class())
cdo.get_component_by_class(u.GunnerDodgeComponent).set_editor_property('roll_montage',roll)
u.BlueprintEditorLibrary.compile_blueprint(character)
for obj in [roll,action,context,config,character]:
    if not lib.save_loaded_asset(obj):raise RuntimeError(f'Could not save {obj}')
u.log('GUNNER_DODGE_INSTALLED')
