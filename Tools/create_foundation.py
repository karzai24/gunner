"""Run in Unreal Editor Python. Creates M0 assets once; refuses to overwrite authored content."""
import unreal as u

ROOT = '/Game/Gunner'
assets = u.AssetToolsHelpers.get_asset_tools()
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)

def asset(name, folder, cls, factory):
    path = f'{ROOT}/{folder}/{name}'
    if u.EditorAssetLibrary.does_asset_exist(path):
        raise RuntimeError(f'Refusing to replace {path}; restore from Git or choose a new output path')
    result = assets.create_asset(name, f'{ROOT}/{folder}', cls, factory)
    if not result:
        raise RuntimeError(f'Could not create {path}')
    return result

def data(name, cls):
    factory = u.DataAssetFactory()
    factory.set_editor_property('data_asset_class', cls)
    return asset(name, 'Input', cls, factory)

def blueprint(name, folder, parent):
    factory = u.BlueprintFactory()
    factory.set_editor_property('parent_class', parent)
    return asset(name, folder, u.Blueprint, factory)

def save(obj):
    u.EditorAssetLibrary.save_loaded_asset(obj)

if u.EditorAssetLibrary.does_asset_exist(f'{ROOT}/Maps/L_Foundation'):
    raise RuntimeError('Foundation already exists; this is a one-time bootstrap, not a map regeneration tool')

move = data('IA_Move', u.InputAction)
mouse = data('IA_MouseLook', u.InputAction)
stick = data('IA_StickLook', u.InputAction)
traverse = data('IA_Traverse', u.InputAction)
for action in (move, mouse, stick):
    action.set_editor_property('value_type', u.InputActionValueType.AXIS2D)
move.set_editor_property('accumulation_behavior', u.InputActionAccumulationBehavior.CUMULATIVE)
context = data('IMC_Foundation', u.InputMappingContext)
# Assign mapping structs as a batch: MapKey's returned Python struct may be a copy.
mappings = []
def mapping(action, key, negate=False, swizzle=False, deadzone=False):
    modifiers = []
    if negate:
        modifiers.append(u.new_object(u.InputModifierNegate, outer=context))
    if swizzle:
        modifier = u.new_object(u.InputModifierSwizzleAxis, outer=context)
        modifier.set_editor_property('order', u.InputAxisSwizzle.YXZ)
        modifiers.append(modifier)
    if deadzone:
        modifiers.append(u.new_object(u.InputModifierDeadZone, outer=context))
    item = u.EnhancedActionKeyMapping()
    item.set_editor_property('action', action)
    input_key = u.Key()
    input_key.set_editor_property('key_name', key)
    item.set_editor_property('key', input_key)
    item.set_editor_property('modifiers', modifiers)
    mappings.append(item)

mapping(move, 'W', swizzle=True)
mapping(move, 'S', negate=True, swizzle=True)
mapping(move, 'D')
mapping(move, 'A', negate=True)
mapping(move, 'Gamepad_Left2D', deadzone=True)
mapping(mouse, 'MouseX')
mapping(mouse, 'MouseY', swizzle=True)
mapping(stick, 'Gamepad_Right2D', deadzone=True)
mapping(traverse, 'SpaceBar')
mapping(traverse, 'Gamepad_FaceButton_Bottom')
context.set_editor_property('default_key_mappings', u.InputMappingContextMappingData(mappings=mappings))
config = data('DA_FoundationInput', u.GunnerInputConfig)
for field, value in [('mapping_context',context),('move',move),('mouse_look',mouse),('stick_look',stick),('traverse',traverse)]:
    config.set_editor_property(field,value)
for obj in (move, mouse, stick, traverse, context, config):
    save(obj)

character = blueprint('BP_Warden', 'Characters', u.GunnerCharacter)
u.BlueprintEditorLibrary.compile_blueprint(character)
cdo = u.get_default_object(character.generated_class())
cdo.set_editor_property('input_config', config)
mesh = cdo.get_component_by_class(u.SkeletalMeshComponent)
mesh.set_skeletal_mesh_asset(u.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple'))
mesh.set_anim_instance_class(u.load_class(None, '/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C'))
u.BlueprintEditorLibrary.compile_blueprint(character)
save(character)
mode = blueprint('BP_GunnerGameMode', 'Core', u.GunnerGameMode)
u.BlueprintEditorLibrary.compile_blueprint(mode)
u.get_default_object(mode.generated_class()).set_editor_property('default_pawn_class', character.generated_class())
u.BlueprintEditorLibrary.compile_blueprint(mode)
save(mode)

materials = {}
for name, rgb in [('Floor',(0.14,0.17,0.19)),('Wall',(0.24,0.29,0.31)),('LowCover',(0.5,0.36,0.12)),('HighCover',(0.16,0.35,0.37)),('Landmark',(0.13,0.46,0.49))]:
    mat = asset(f'M_{name}', 'Materials', u.Material, u.MaterialFactoryNew())
    color = u.MaterialEditingLibrary.create_material_expression(mat,u.MaterialExpressionConstant3Vector)
    color.set_editor_property('constant', u.LinearColor(*rgb))
    u.MaterialEditingLibrary.connect_material_property(color,'',u.MaterialProperty.MP_BASE_COLOR)
    roughness = u.MaterialEditingLibrary.create_material_expression(mat,u.MaterialExpressionConstant)
    roughness.set_editor_property('r',0.85)
    u.MaterialEditingLibrary.connect_material_property(roughness,'',u.MaterialProperty.MP_ROUGHNESS)
    u.MaterialEditingLibrary.recompile_material(mat)
    save(mat)
    materials[name]=mat

if not levels.new_level(f'{ROOT}/Maps/L_Foundation'):
    raise RuntimeError('Could not create foundation level')
cube = u.load_asset('/Engine/BasicShapes/Cube')
def block(name, position, size, material):
    actor=actors.spawn_actor_from_class(u.StaticMeshActor,u.Vector(*position))
    actor.set_actor_label(name)
    comp=actor.static_mesh_component
    comp.set_static_mesh(cube)
    comp.set_material(0,materials[material])
    actor.set_actor_scale3d(u.Vector(*(v/100 for v in size)))
    return actor
block('Floor_44x32m',(0,0,-25),(4400,3200,50),'Floor')
block('NorthWall',(0,1625,150),(4500,50,300),'Wall')
block('SouthWall',(0,-1625,150),(4500,50,300),'Wall')
block('EastWall',(2225,0,150),(50,3200,300),'Wall')
block('WestWall',(-2225,0,150),(50,3200,300),'Wall')
block('LowCover_115cm',(0,-700,57.5),(400,100,115),'LowCover')
block('HighCover_180cm',(500,700,90),(400,100,180),'HighCover')
block('ReactorMetricMarker',(900,0,125),(180,180,250),'Landmark')
start=actors.spawn_actor_from_class(u.PlayerStart,u.Vector(-1500,0,95),u.Rotator(0,0,0))
start.set_actor_label('PlayerStart_Foundation')
light=actors.spawn_actor_from_class(u.DirectionalLight,u.Vector(0,0,800),u.Rotator(-45,-35,0))
light.light_component.set_editor_property('intensity',3.0)
light.light_component.set_mobility(u.ComponentMobility.MOVABLE)
sky=actors.spawn_actor_from_class(u.SkyLight,u.Vector(0,0,500))
sky.light_component.set_mobility(u.ComponentMobility.MOVABLE)
sky.light_component.set_editor_property('intensity',1.0)
actors.spawn_actor_from_class(u.SkyAtmosphere,u.Vector())
world= u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode', mode.generated_class())
levels.save_current_level()
u.EditorAssetLibrary.save_directory(ROOT,only_if_is_dirty=True,recursive=True)
u.log('GUNNER_BOOTSTRAP_COMPLETE')
