from pathlib import Path
import json
import unreal as u
root='/Game/Gunner/Motion'
report={'weapons':{},'graph':None}
bp=u.load_asset(root+'/Characters/BP_WardenMotion')
component=u.get_default_object(bp.generated_class()).get_component_by_class(u.SkeletalMeshComponent)
cls=component.get_editor_property('anim_class')
assert cls.get_path_name()==root+'/Animation/ABP_WardenMotionPolish.ABP_WardenMotionPolish_C'
report['graph']=cls.get_path_name()
settings=u.get_default_object(cls)
for field in ['blind_fire_pose_ready','crouch_reload_pose_ready','crouch_equip_pose_ready','crouch_dry_fire_pose_ready']:
 assert settings.get_editor_property(field)
for kind in ['Rifle','Pistol']:
 data=u.load_asset(root+'/Weapons/DA_'+kind)
 items={}
 for field in ['jump_start_montage','jump_land_montage','dry_fire_montage','alternate_melee_montage']:
  a=data.get_editor_property(field)
  assert a
  slot=str(a.get_editor_property('slot_anim_tracks')[0].get_editor_property('slot_name'))
  assert slot==('UpperBody' if field=='dry_fire_montage' else 'FullBody')
  items[field]={'asset':a.get_path_name(),'slot':slot}
 for action,source_name in [('JumpStart','Start'),('JumpLand','RecoveryAdditive')]:
  a=u.load_asset(root+'/Animation/InPlace/A_'+kind+'_'+action)
  original=u.load_asset('/Game/Characters/Mannequins/Anims/'+kind+'/Jump/MM_'+kind+'_Jump_'+source_name)
  assert not a.get_editor_property('enable_root_motion')
  assert a.get_editor_property('force_root_lock')
  assert a.get_editor_property('root_motion_root_lock')==u.RootMotionRootLock.REF_POSE
  assert original.get_editor_property('enable_root_motion')
  items[action]={'copy':a.get_path_name(),'root_motion':False,'root_lock':'REF_POSE','original_root_motion':True}
 report['weapons'][kind]=items
Path(u.Paths.project_saved_dir(),'motion_polish_persisted.json').write_text(json.dumps(report,indent=2))
u.log('GUNNER_POLISH_PERSISTED_ASSETS_PASS')
