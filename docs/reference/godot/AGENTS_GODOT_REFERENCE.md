# AGENTS.md — Commercial Game Engine Architecture & Godot 4 3D TPS Specification

> **Target Audience:** Autonomous Coding Agents, LLM Technical Assistants, and Lead Game Engine Architects.  
> **Target Engine/Languages:** Godot Engine 4.x (GDScript / C# GDExtension) & Core C++ Engine Paradigms.  
> **Primary Mandate:** When generating, refactoring, or evaluating engine code, prioritize **cache locality, zero-allocation runtime tick paths, cache-friendly data structures, explicit state machine hierarchies, spatial coherence, and thread-safe decoupling** over high-level Object-Oriented abstraction (e.g., dynamic virtual dispatch, scattered heap pointers, deep class hierarchies).

---

## 1. Executive Directives & System Mandates

1. **Memory Allocation in Ticks:** Zero dynamic allocations (`new`, `malloc`, `std::make_shared`, `instantiate()`, `.new()`) inside frame `Update()`, `FixedUpdate()`, `Render()`, `_process()`, or `_physics_process()` loops. All buffers and actors must be pre-allocated via Arena, Fixed Pool, Node Pools, or Frame Allocators.
2. **Data-Oriented Preference:** Default to Struct-of-Arrays (SoA) or Array-of-Structs-of-Arrays (AoSoA) over standard Array-of-Structs (AoS) for systems iterating over > 100 elements per frame. In Godot, isolate static/gameplay tables in custom `Resource` assets rather than storing data arrays inside Node scripts.
3. **Decoupling & Threading Boundaries:** Decouple distant systems via value queues, double buffers, or pub/sub event buses (`EventBus`). Do not lock mutexes in frame-critical iteration loops.
4. **Execution Phasing:** Strictly maintain the order of execution phases:
   `Input / _unhandled_input -> Network Poll -> Fixed Physics Simulation (_physics_process @ 60Hz) -> Early Update -> Main Logic Update (_process) -> Late Update -> Render State Double-Buffer Swap -> Render Thread Dispatch`.
5. **Godot Communication Idiom ("Call Down, Signal Up"):** Parent nodes call child functions directly via node references or `@onready` paths; child nodes communicate up to parents solely via strongly-typed `signal` definitions.

---

## 2. Universal State Machine Architecture

Every active system in the game MUST use an explicit Node-based Finite State Machine (FSM). Do NOT write giant `match` or `if/else` chains for state management.

### 2.1 Base FSM Implementation

```gdscript
class_name State extends Node

signal state_transitioned(from_state: State, to_state_name: StringName, msg: Dictionary)

func enter(_msg: Dictionary = {}) -> void:
    pass

func exit() -> void:
    pass

func update(_delta: float) -> void:
    pass

func physics_update(_delta: float) -> void:
    pass
```

```gdscript
class_name FiniteStateMachine extends Node

@export var initial_state: State
var current_state: State
var states: Dictionary = {}

func _ready() -> void:
    for child in get_children():
        if child is State:
            states[child.name.to_lower()] = child
            child.state_transitioned.connect(_on_state_transitioned)
            
    if initial_state:
        initial_state.enter()
        current_state = initial_state

func _physics_process(delta: float) -> void:
    if current_state:
        current_state.physics_update(delta)

func _process(delta: float) -> void:
    if current_state:
        current_state.update(delta)

func _on_state_transitioned(from: State, to_name: StringName, msg: Dictionary) -> void:
    if from != current_state:
        return
    var new_state: State = states.get(to_name.to_lower())
    if not new_state:
        return
    if current_state:
        current_state.exit()
    new_state.enter(msg)
    current_state = new_state
```

### 2.2 Layered State Machine Matrix

1. **Global Game Flow State Machine (`GameManager` Autoload):**
   * *States:* `BootState`, `MainMenuState`, `LoadingState`, `GameplayState`, `PauseState`, `GameOverState`.
   * *Purpose:* Manages application lifecycle, level streaming triggers, mouse capture modes, and global game pause states.
2. **Entity Locomotion & Action State Machine (`CharacterBody3D`):**
   * *States:* `IdleState`, `MoveState`, `SprintState`, `AirborneState`, `CoverState`, `VaultState`.
   * *Purpose:* Separates kinematic velocity updates, friction curves, step sounds, and animation parameters per movement mode.
3. **Weapon Operational State Machine:**
   * *States:* `ReadyState`, `FiringState`, `ReloadingState`, `EmptyState`, `OverheatedState`.
   * *Purpose:* Encapsulates weapon cycles, rate-of-fire timers, ammo consumption, and prevents invalid inputs (e.g., firing mid-reload).
4. **AI Behavioral State Machine:**
   * *States:* `PatrolState`, `InvestigateState`, `CombatFlankState`, `RetreatState`.
   * *Purpose:* Interfaces with Godot's `NavigationServer3D` to manage sight perception, pathfinding, and cover evaluation.
5. **UI Screen Navigation State Machine:**
   * *States:* `HUDState`, `InventoryUIState`, `SettingsUIState`, `PauseMenuState`.
   * *Purpose:* Manages stacked menu navigation, modal dialogs, and dynamic UI focus controls.

---

## 3. Core Engine Sequencing & Messaging Patterns

### 3.1 Fixed-Step Game Loop with Frame Interpolation
Decouples engine tick rate and physics evaluation from user hardware clock rate using high-resolution time steps and accumulator interpolation.

```cpp
struct EngineLoop {
    double previous_time = GetHighResTime();
    double lag = 0.0;
    const double MS_PER_UPDATE = 1.0 / 60.0; // Fixed 60 Hz simulation

    void RunFrame() {
        double current_time = GetHighResTime();
        double elapsed = current_time - previous_time;
        previous_time = current_time;
        lag += elapsed;

        ProcessInput();

        while (lag >= MS_PER_UPDATE) {
            FixedUpdate(MS_PER_UPDATE);
            lag -= MS_PER_UPDATE;
        }

        double alpha = lag / MS_PER_UPDATE;
        Render(alpha);
    }
};
```

### 3.2 Double Buffering
Prevents state race conditions between concurrent worker threads (e.g., simulation writing next frame while render pipeline reads current frame).

```cpp
template<typename T>
class DoubleBuffer {
    T buffers[2];
    uint32_t current_read = 0;
public:
    const T& GetReadBuffer() const { return buffers[current_read]; }
    T& GetWriteBuffer() { return buffers[current_read ^ 1]; }
    void Swap() { current_read ^= 1; }
};
```

### 3.3 Command Pattern (Rebindable Input Buffering)
Encapsulates actions into value objects to facilitate rebindable inputs, action queues, multi-level undo/redo, and deterministic frame rollback.

```gdscript
class_name InputCommand extends RefCounted

var execute_callable: Callable

func _init(p_callable: Callable) -> void:
    execute_callable = p_callable

func execute(actor: Node3D) -> void:
    if execute_callable.is_valid():
        execute_callable.call(actor)
```

### 3.4 Global Event Bus (`EventBus.gd` Autoload)
Distant or unrelated systems (Combat -> UI, Enemy Death -> Audio Engine) communicate strictly through an Autoload Event Bus to prevent brittle dependencies.

```gdscript
class_name EventBusSingleton extends Node

# Gameplay Signals
signal player_health_changed(current_health: float, max_health: float)
signal player_died()
signal enemy_killed(enemy_type: StringName, reward_xp: int)
signal weapon_fired(weapon_data: WeaponData, current_ammo: int)
signal ammo_changed(current_ammo: int, reserve_ammo: int)
signal game_paused(is_paused: bool)
```

---

## 4. High-Performance Memory & Component Patterns

### 4.1 Struct-of-Arrays (SoA) Layout
Arranges member data into contiguous primitive arrays matching CPU cache lines (64 bytes), maximizing L1/L2 cache hit rate during batch operations.

```cpp
struct alignas(64) TransformSoA {
    float posX[4096];
    float posY[4096];
    float posZ[4096];
    float velX[4096];
    float velY[4096];
    float velZ[4096];
    size_t count = 0;
};

void UpdateMovementSystem(TransformSoA& t, float dt) {
    for (size_t i = 0; i < t.count; ++i) {
        t.posX[i] += t.velX[i] * dt;
        t.posY[i] += t.velY[i] * dt;
        t.posZ[i] += t.velZ[i] * dt;
    }
}
```

### 4.2 Centralized Node Object Pool (`NodePool3D`)
Pre-instantiates `Node3D` scenes to eliminate heap fragmentation and dynamic memory locks during runtime entity spawning.

```gdscript
class_name NodePool3D extends Node

@export var scene_to_pool: PackedScene
@export var initial_pool_size: int = 100

var _available_pool: Array[Node3D] = []

func _ready() -> void:
    for i in range(initial_pool_size):
        var instance := scene_to_pool.instantiate() as Node3D
        instance.set_process(false)
        instance.set_physics_process(false)
        instance.hide()
        add_child(instance)
        _available_pool.append(instance)

func spawn(spawn_transform: Transform3D) -> Node3D:
    if _available_pool.is_empty():
        var extra := scene_to_pool.instantiate() as Node3D
        add_child(extra)
        extra.global_transform = spawn_transform
        return extra
    
    var obj := _available_pool.pop_back()
    obj.global_transform = spawn_transform
    obj.show()
    obj.set_process(true)
    obj.set_physics_process(true)
    return obj

func recycle(obj: Node3D) -> void:
    obj.hide()
    obj.set_process(false)
    obj.set_physics_process(false)
    _available_pool.append(obj)
```

### 4.3 Custom Resource Data Assets (Type Object Pattern)
Replaces heavy hardcoded script classes with Godot's native lightweight `Resource` data assets.

```gdscript
class_name WeaponData extends Resource

@export var weapon_name: String = "Rifle"
@export var fire_rate: float = 0.1
@export var damage: float = 25.0
@export var recoil_impulse: Vector2 = Vector2(0.1, 0.4)
@export var muzzle_flash_effect: PackedScene
```

### 4.4 Component-Based Combat System
Decouples damage evaluation, health management, and hurtbox collision into atomic, modular components:

```gdscript
class_name HealthComponent extends Node

signal health_depleted()
signal health_changed(current: float, max_hp: float)

@export var max_health: float = 100.0
@onready var current_health: float = max_health

func take_damage(amount: float) -> void:
    current_health = clampf(current_health - amount, 0.0, max_health)
    health_changed.emit(current_health, max_health)
    if current_health <= 0.0:
        health_depleted.emit()
```

```gdscript
class_name HurtboxComponent extends Area3D

@export var health_component: HealthComponent

func receive_hit(damage_amount: float) -> void:
    if health_component:
        health_component.take_damage(damage_amount)
```

---

## 5. Core Engine Manager Subsystems

1. **`AudioManager` (Autoload):** Manages pooled `AudioStreamPlayer` nodes across `Master`, `SFX`, `Music`, and `Voice` channels to eliminate dynamic audio node instantiation during combat events.
2. **`LevelManager` (Autoload):** Executes non-blocking background scene loads via `ResourceLoader.load_threaded_request()` to prevent engine stutter during level transitions.
3. **`CameraManager` (Autoload):** Orchestrates global camera targets, field of view adjustments, screen shake impulses, and smooth blending.
4. **`SaveManager` (Autoload):** Encapsulates player progress and game settings serialization using encrypted `.tres` custom resources.
5. **Passive UI Architecture (MVC):** UI `Control` nodes must **never** hold direct references to `CharacterBody3D` or gameplay actors. UI elements act purely as passive views listening to signals emitted by the `EventBus`.

```gdscript
class_name HealthBarUI extends TextureProgressBar

func _ready() -> void:
    EventBus.player_health_changed.connect(_on_player_health_changed)

func _on_player_health_changed(current_health: float, max_health: float) -> void:
    max_value = max_health
    value = current_health
```

---

## 6. Specialized 3D Third-Person Shooter (TPS) Systems

### 6.1 SpringArm3D Camera Rig & Occlusion Probing
Uses Godot's native `SpringArm3D` fitted with a `SphereShape3D` shape cast probe to prevent camera geometry clipping while supporting shoulder offsets and aim zooming.

```gdscript
class_name TPSCameraRig extends Node3D

@onready var spring_arm: SpringArm3D = $SpringArm3D
@onready var camera: Camera3D = $SpringArm3D/Camera3D

@export var default_offset: Vector3 = Vector3(0.5, 0.4, 0.0)
@export var aim_offset: Vector3 = Vector3(0.3, 0.2, 0.0)

func _ready() -> void:
    spring_arm.add_excluded_object(get_parent().get_rid())
    var sphere_probe := SphereShape3D.new()
    sphere_probe.radius = 0.2
    spring_arm.shape = sphere_probe

func set_aiming(is_aiming: bool) -> void:
    var target := aim_offset if is_aiming else default_offset
    var tween := create_tween().set_parallel(true)
    tween.tween_property(camera, "position", target, 0.15)
    tween.tween_property(camera, "fov", 55.0 if is_aiming else 75.0, 0.15)
```

### 6.2 Two-Stage Muzzle-to-Crosshair Aim Calibration Solver
Performs a two-stage direct world `PhysicsRayQueryParameters3D` raycast from screen center to muzzle transform to guarantee crosshair aim precision.

```gdscript
class_name MuzzleAimSolver extends Node3D

static func get_aim_vector(muzzle_node: Node3D, camera: Camera3D, max_dist: float = 1000.0) -> Vector3:
    var space_state := muzzle_node.get_world_3d().direct_space_state
    var screen_center := camera.get_viewport().get_visible_rect().size / 2.0
    
    # Stage 1: Raycast from camera center into world
    var cam_ray_origin := camera.project_ray_origin(screen_center)
    var cam_ray_dir := camera.project_ray_normal(screen_center)
    
    var query := PhysicsRayQueryParameters3D.create(cam_ray_origin, cam_ray_origin + cam_ray_dir * max_dist)
    query.exclude = [muzzle_node.get_parent().get_rid()]
    var result := space_state.intersect_ray(query)
    
    var target_point: Vector3 = result.position if not result.is_empty() else (cam_ray_origin + cam_ray_dir * max_dist)
    
    # Stage 2: Muzzle-to-Target normalized vector
    return (target_point - muzzle_node.global_position).normalized()
```

### 6.3 Dynamic Geometry Cover Detector
Evaluates obstacle heights using dual forward ray casts:

```gdscript
class_name CoverDetector extends Node3D

enum CoverState { NONE, LOW, HIGH }

func evaluate_cover(player: CharacterBody3D, distance: float = 1.5) -> CoverState:
    var space_state := player.get_world_3d().direct_space_state
    var forward := -player.global_transform.basis.z
    
    # Low ray trace (chest)
    var chest_pos := player.global_position + Vector3(0, 1.0, 0)
    var low_query := PhysicsRayQueryParameters3D.create(chest_pos, chest_pos + forward * distance)
    var low_result := space_state.intersect_ray(low_query)
    
    if low_result.is_empty():
        return CoverState.NONE
        
    # High ray trace (head)
    var head_pos := player.global_position + Vector3(0, 1.8, 0)
    var high_query := PhysicsRayQueryParameters3D.create(head_pos, head_pos + forward * distance)
    var high_result := space_state.intersect_ray(high_query)
    
    return CoverState.HIGH if not high_result.is_empty() else CoverState.LOW
```

### 6.4 Server Lag Compensation Rewind Buffer
Server-side rewind buffer for precise multiplayer hitscan hit validation.

```cpp
struct FrameSnapshot {
    uint32_t frame_number;
    double timestamp;
    std::unordered_map<uint64_t, BoundingBox> entity_hitboxes;
};

class LagCompensationSystem {
    std::deque<FrameSnapshot> history_buffer; // Stores ~500ms past states
    
public:
    bool VerifyHit(uint64_t target_id, const Ray& client_aim_ray, double client_timestamp) {
        FrameSnapshot state = GetInterpolatedHistory(client_timestamp);
        auto it = state.entity_hitboxes.find(target_id);
        if (it != state.entity_hitboxes.end()) {
            return Physics::RayIntersectsBox(client_aim_ray, it->second);
        }
        return false;
    }
};
```

---

## 7. Reference Pattern Matrix

| Pattern Name | Architecture Mechanism | Primary Engine Application |
| :--- | :--- | :--- |
| **Data Locality (SoA)** | Alignment & contiguous memory arrays | L1/L2 cache hit optimization |
| **Object Pool** | `Array[Node3D]` pre-instantiation | Bullets, projectiles, particle pooling |
| **Type Object** | Custom `Resource` scripts | Data-driven weapon & ammo tables |
| **Spring Arm Camera** | `SpringArm3D` + `SphereShape3D` | Camera collision & shoulder swapping |
| **Aim Alignment** | `PhysicsRayQueryParameters3D` | Two-stage muzzle target calibration |
| **Locomotion Blending** | `AnimationTree` + `BlendSpace2D` | Lower-body / Upper-body Aim Additive pose |
| **Service Locator / Bus** | Autoload Singletons | Global Sound Manager, Event Bus |
| **Lag Compensation** | Historical Ring Buffer | Server-side hitscan rewind validation |

---

## 8. Mandatory Project Directory Structure

```text
res://
├── assets/          # Raw 3D models (.gltf), textures, audio files
├── autoload/        # Global Singletons (EventBus.gd, GameManager.gd, AudioManager.gd, LevelManager.gd)
├── core/            # Base engine framework (HFSM base nodes, Poolers, Solvers)
├── data/            # Custom Resource definitions (.tres)
└── scenes/          # Combined, feature-grouped game scenes
    ├── player/      # Player.tscn, PlayerController.gd, states/
    ├── enemies/     # Enemy base scenes, AI behavior states/
    ├── weapons/     # Weapon meshes, bullet pools, weapon resource assets
    └── ui/          # HUD, Menus, Pause systems
```

---

## 9. Comprehensive AAA AI Pre-Commit Self-Audit Checklist

Before completing any task, generating code, or refactoring engine systems, the AI agent must pass the following verification gates:

- [ ] **Zero Tick Allocation:** Are `_process()` and `_physics_process()` completely free of dynamic allocations (`instantiate()`, `.new()`, array re-sizing)?
- [ ] **Node Decoupling:** Are child-to-parent communication paths handled via `signal`, and parent-to-child paths handled via direct method invocation?
- [ ] **State Machine Enforcement:** Are complex states (Player locomotion, Weapons, Game Flow, AI, UI) split into explicit state nodes extending `State` rather than using `match` statements?
- [ ] **Event Routing:** Is cross-system communication (UI updates, audio triggers, achievements) routed cleanly through `EventBus`?
- [ ] **UI Passivity:** Is the UI purely passive, updating strictly from signal parameters from `EventBus` rather than directly inspecting gameplay nodes?
- [ ] **Atomic Combat:** Are damage evaluation, collision detection, and health logic isolated into modular `HitboxComponent`, `HurtboxComponent`, and `HealthComponent` nodes?
- [ ] **Clean Serialization:** Does save/load state serialize cleanly into `.tres` `Resource` files rather than writing directly to Node attributes?