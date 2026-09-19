"""One-time motion sandbox composition. Preserves the M0 assets and refuses replacement.
Run after retarget_motion.py in an editor Python commandlet (NullRHI is sufficient).
"""
import math
from pathlib import Path
import unreal as u

ROOT = '/Game/Gunner/Motion'
tools = u.AssetToolsHelpers.get_asset_tools()
lib = u.EditorAssetLibrary
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)

if lib.does_directory_exist(ROOT):
    raise RuntimeError('Motion assets already exist. Refusing to overwrite authored content.')

def load(path):
    result = u.load_asset(path)
    if not result:
        raise RuntimeError(f'Missing required asset: {path}')
    return result

def save(obj):
    if not obj or not lib.save_loaded_asset(obj):
        raise RuntimeError(f'Failed asset save: {obj}')
    return obj

def create(name, folder, cls, factory):
    path = f'{ROOT}/{folder}/{name}'
    if lib.does_asset_exist(path):
        raise RuntimeError(f'Refusing overwrite: {path}')
    obj = tools.create_asset(name, f'{ROOT}/{folder}', cls, factory)
    if not obj:
        raise RuntimeError(f'Failed creation: {path}')
    return obj

def data(name, folder, cls):
    factory = u.DataAssetFactory()
    factory.set_editor_property('data_asset_class', cls)
    return create(name, folder, cls, factory)

def bp(name, folder, parent):
    factory = u.BlueprintFactory()
    factory.set_editor_property('parent_class', parent)
    obj = create(name, folder, u.Blueprint, factory)
    u.BlueprintEditorLibrary.compile_blueprint(obj)
    return obj

skel = load('/Game/Characters/Mannequins/Meshes/SK_Mannequin')
mesh = load('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
ANIM = '/Game/Characters/Mannequins/Anims'
RETARGET = '/Game/Gunner/Animation/Retargeted/Manny'
retarget_paths = lib.list_assets(RETARGET, recursive=True)
def retargeted(suffix):
    matches = [p for p in retarget_paths if p.split('.')[-1].endswith(suffix)]
    if len(matches) != 1:
        raise RuntimeError(f'Expected one retargeted {suffix}: {matches}')
    return load(matches[0])

def locomotion(weapon):
    clips = [load(f'{ANIM}/{weapon}/MF_{weapon}_Idle_ADS')]
    points = [u.Vector()]
    directions = [('Fwd',1,0),('Fwd_Right',1,1),('Right',0,1),('Bwd_Right',-1,1),
                  ('Bwd',-1,0),('Bwd_Left',-1,-1),('Left',0,-1),('Fwd_Left',1,-1)]
    for gait,speed in [('Walk',180.0),('Jog',450.0)]:
        for direction,x,y in directions:
            scale = speed / math.sqrt(x*x+y*y)
            clips.append(load(f'{ANIM}/{weapon}/{gait}/MF_{weapon}_{gait}_{direction}'))
            points.append(u.Vector(x*scale,y*scale,0))
    result = u.GunnerAnimationBuilder.create_directional_blend_space(
        f'{ROOT}/Animation/BS_{weapon}', skel, mesh, clips, points, 450.0)
    if not result: raise RuntimeError(f'Failed {weapon} blend space')
    return result

rifle_bs = locomotion('Rifle')
pistol_bs = locomotion('Pistol')
crouch_bs = u.GunnerAnimationBuilder.create_directional_blend_space(
    f'{ROOT}/Animation/BS_Crouch', skel, mesh,
    [retargeted('Crouch_Idle_Loop'), retargeted('Crouch_Fwd_Loop')],
    [u.Vector(), u.Vector(140,0,0)], 140.0)
anim = u.GunnerAnimationBuilder.create_locomotion_blueprint(
    f'{ROOT}/Animation/ABP_WardenMotion', skel, mesh, rifle_bs, pistol_bs,
    load(f'{ANIM}/Rifle/Jump/MM_Rifle_Jump_Fall_Loop'), load(f'{ANIM}/Pistol/Jump/MM_Pistol_Jump_Fall_Loop'),
    crouch_bs, retargeted('Sprint_Loop'), load(f'{ANIM}/Rifle/AIM/AO_Rifle'), load(f'{ANIM}/Pistol/Aim/AO_Pistol'))
if not anim: raise RuntimeError('Animation graph creation failed')
save(skel)  # New project montage-slot metadata only; animation/mesh tracks are unchanged.

def montage(name, clip, slot='UpperBody'):
    factory = u.AnimMontageFactory()
    factory.set_editor_property('target_skeleton', skel)
    factory.set_editor_property('source_animation', clip)
    obj = create(name, 'Animation/Montages', u.AnimMontage, factory)
    tracks = obj.get_editor_property('slot_anim_tracks')
    track = tracks[0]
    track.set_editor_property('slot_name', slot)
    tracks[0] = track  # Unreal returns a struct copy for an array element.
    obj.set_editor_property('slot_anim_tracks', tracks)
    if str(obj.get_editor_property('slot_anim_tracks')[0].get_editor_property('slot_name')) != slot:
        raise RuntimeError(f'Montage slot assignment failed: {name}')
    for prop,seconds in [('blend_in',0.08),('blend_out',0.12)]:
        blend = obj.get_editor_property(prop)
        blend.set_editor_property('blend_time', seconds)
        obj.set_editor_property(prop,blend)
    return save(obj)

melee = montage('AM_MeleeJab', retargeted('Punch_Jab'), 'FullBody')
roll = montage('AM_DodgeRoll', retargeted('Roll'), 'FullBody')
roll.set_editor_property('rate_scale',1.65)
save(roll)
weapons = {}
for kind,capacity,reserve,interval in [('Rifle',30,240,0.12),('Pistol',12,120,0.27)]:
    definition = data(f'DA_{kind}', 'Weapons', u.GunnerWeaponData)
    fields = dict(content_id='weapon_arc_carbine' if kind=='Rifle' else 'weapon_coil_pistol',
                  display_name='ARC CARBINE' if kind=='Rifle' else 'COIL PISTOL',
                  weapon_kind=u.GunnerWeaponKind.RIFLE if kind=='Rifle' else u.GunnerWeaponKind.PISTOL,
                  gun_mesh=load(f'/Game/Weapons/{kind}/Meshes/SM_{kind}'),
                  magazine_capacity=capacity, initial_reserve=reserve, automatic=kind=='Rifle',
                  fire_interval=interval, damage=20.0 if kind=='Rifle' else 35.0, melee_montage=melee)
    for action in ['Fire','Reload','Equip']:
        fields[f'{action.lower()}_montage'] = montage(f'AM_{kind}_{action}', load(f'{ANIM}/{kind}/MM_{kind}_{action}'))
    # Both imported weapons point +Y in mesh space. Solve the rest grip from the
    # actual authored armed idle, rather than assuming the hand bone points forward.
    pose=u.AnimPoseExtensions.get_anim_pose_at_time(load(f'{ANIM}/{kind}/MF_{kind}_Idle_ADS'),0.0,u.AnimPoseEvaluationOptions())
    hand=u.AnimPoseExtensions.get_bone_pose(pose,'hand_r',u.AnimPoseSpaces.WORLD)
    grip=u.Transform()
    q=hand.rotation
    grip.set_editor_property('rotation',u.Quat(-q.x,-q.y,-q.z,q.w))
    fields['grip_transform'] = grip
    fields['muzzle_offset'] = u.Vector(0,51 if kind=='Rifle' else 21,9)
    for name,value in fields.items(): definition.set_editor_property(name,value)
    weapons[kind] = save(definition)

config = data('DA_MotionInput','Input',u.GunnerInputConfig)
context = data('IMC_Motion','Input',u.InputMappingContext)
actions = {}
for field in ['Move','MouseLook','StickLook','Traverse']:
    actions[field] = load(f'/Game/Gunner/Input/IA_{field}')
for field in ['Aim','Fire','Reload','CrouchToggle','Sprint','ShoulderSwap','Rifle','Pistol','Melee','Jump','Dodge']:
    actions[field] = save(data(f'IA_{field}','Input',u.InputAction))
mappings = []
def bind(action,key,negate=False,swizzle=False,deadzone=False):
    item=u.EnhancedActionKeyMapping()
    item.set_editor_property('action',actions[action])
    k=u.Key(); k.set_editor_property('key_name',key)
    item.set_editor_property('key',k)
    modifiers=[]
    if negate: modifiers.append(u.new_object(u.InputModifierNegate,outer=context))
    if swizzle:
        modifier=u.new_object(u.InputModifierSwizzleAxis,outer=context)
        modifier.set_editor_property('order',u.InputAxisSwizzle.YXZ)
        modifiers.append(modifier)
    if deadzone: modifiers.append(u.new_object(u.InputModifierDeadZone,outer=context))
    item.set_editor_property('modifiers',modifiers)
    mappings.append(item)
bind('Move','W',swizzle=True)
bind('Move','S',negate=True,swizzle=True)
bind('Move','D')
bind('Move','A',negate=True)
bind('Move','Gamepad_Left2D',deadzone=True)
bind('MouseLook','MouseX')
bind('MouseLook','MouseY',swizzle=True)
bind('StickLook','Gamepad_Right2D',deadzone=True)
bind('Traverse','SpaceBar')
bind('Traverse','Gamepad_FaceButton_Bottom')
for action,keys in {
    'Aim':['RightMouseButton','Gamepad_LeftTrigger'], 'Fire':['LeftMouseButton','Gamepad_RightTrigger'],
    'Reload':['R','Gamepad_FaceButton_Left'], 'CrouchToggle':['C','LeftControl','Gamepad_FaceButton_Right'],
    'Sprint':['LeftShift','Gamepad_LeftThumbstick'], 'ShoulderSwap':['Q','Gamepad_RightShoulder'],
    'Rifle':['One','Gamepad_DPad_Up'], 'Pistol':['Two','Gamepad_DPad_Down'],
    'Melee':['F','Gamepad_RightThumbstick'], 'Jump':['J'], 'Dodge':['E','Gamepad_LeftShoulder']}.items():
    for key in keys: bind(action,key)
context.set_editor_property('default_key_mappings',u.InputMappingContextMappingData(mappings=mappings))
config.set_editor_property('mapping_context',save(context))
fields={'MouseLook':'mouse_look','StickLook':'stick_look','CrouchToggle':'crouch_toggle','ShoulderSwap':'shoulder_swap'}
for name,action in actions.items(): config.set_editor_property(fields.get(name,name.lower()),action)
save(config)

motion = data('DA_MotionSettings','Movement',u.GunnerMotionSettings)
for name,value in [('crouch_ready',True),('sprint_ready',True),('cover_ready',True),('directional_crouch_ready',False)]:
    motion.set_editor_property(name,value)
save(motion)
character = bp('BP_WardenMotion','Characters',u.GunnerCharacter)
cdo = u.get_default_object(character.generated_class())
cdo.set_editor_property('input_config', config)
cdo.set_editor_property('motion_settings', motion)
sk_comp = cdo.get_component_by_class(u.SkeletalMeshComponent)
sk_comp.set_skeletal_mesh_asset(mesh)
sk_comp.set_anim_instance_class(anim.generated_class())
combat = cdo.get_component_by_class(u.GunnerCombatComponent)
combat.set_editor_property('rifle_data', weapons['Rifle'])
combat.set_editor_property('pistol_data', weapons['Pistol'])
cdo.get_component_by_class(u.GunnerDodgeComponent).set_editor_property('roll_montage',roll)
u.BlueprintEditorLibrary.compile_blueprint(character); save(character)
mode = bp('BP_MotionGameMode','Core',u.GunnerGameMode)
mode_cdo = u.get_default_object(mode.generated_class())
mode_cdo.set_editor_property('default_pawn_class',character.generated_class())
mode_cdo.set_editor_property('hud_class',u.GunnerHUD)
u.BlueprintEditorLibrary.compile_blueprint(mode); save(mode)

map_path=f'{ROOT}/Maps/L_MotionRange'
if not lib.duplicate_asset('/Game/Gunner/Maps/L_Foundation',map_path):
    raise RuntimeError('Could not duplicate foundation into the motion range')
levels.load_level(map_path)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode',mode.generated_class())
cube=load('/Engine/BasicShapes/Cube')
target_bp=bp('BP_RangeTarget','Targets',u.GunnerTarget)
target_cdo=u.get_default_object(target_bp.generated_class())
target_mesh=target_cdo.get_component_by_class(u.StaticMeshComponent)
target_mesh.set_static_mesh(cube)
target_mesh.set_relative_scale3d(u.Vector(0.35,0.8,1.8))
target_mesh.set_material(0,load('/Game/Gunner/Materials/M_Landmark'))
u.BlueprintEditorLibrary.compile_blueprint(target_bp); save(target_bp)
for i,y in enumerate([-1000,0,1000],1):
    target=actors.spawn_actor_from_class(target_bp.generated_class(),u.Vector(1400,y,90))
    target.set_actor_label(f'RangeTarget_{i}')
    target.set_editor_property('target_name',f'TARGET {i:02}')
for text,point in [('MOVEMENT / WEAPON RANGE',(-1200,-1400,240)),
                   ('LOW COVER  /  SPACE TO ATTACH',(0,-700,180)),
                   ('HIGH COVER  /  Q TO SWITCH SHOULDER',(500,700,235))]:
    actor=actors.spawn_actor_from_class(u.TextRenderActor,u.Vector(*point),u.Rotator(pitch=0,yaw=180,roll=0))
    actor.text_render.set_text(text)
    actor.text_render.set_world_size(22)
    actor.text_render.set_text_render_color(u.Color(190,220,215,255))
levels.save_current_level()
lib.save_directory(ROOT,only_if_is_dirty=True,recursive=True)
u.log('GUNNER_MOTION_SANDBOX_CREATED')
