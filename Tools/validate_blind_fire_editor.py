"""Run two real PIE sessions of -GunnerBlindFireSmoke, then close the test editor.

Synthetic input exercises live blind-fire transitions, real evaluated grips and traces.
"""
import time
import unreal as u

levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
performance = u.get_default_object(u.load_class(None, '/Script/UnrealEd.EditorPerformanceSettings'))
was_throttled = performance.get_editor_property('bThrottleCPUWhenNotForeground')
performance.set_editor_property('bThrottleCPUWhenNotForeground', False)
u.SystemLibrary.execute_console_command(editor.get_editor_world(), 't.MaxFPS 60')
backend = u.SystemLibrary.get_console_variable_int_value('Slate.MacUseNewMouseControllerMovement')
u.log(f'GUNNER_BLIND_MAC_NEW_MOUSE_BACKEND value={backend}')
state = {'phase': 'start', 'at': time.monotonic(), 'cycles': 0}


def finish():
    performance.set_editor_property('bThrottleCPUWhenNotForeground', was_throttled)
    u.unregister_slate_post_tick_callback(handle)
    u.SystemLibrary.quit_editor()


def tick(delta):
    elapsed = time.monotonic() - state['at']
    if state['phase'] == 'start' and elapsed > 3:
        levels.editor_request_begin_play()
        state.update(phase='play', at=time.monotonic())
    elif state['phase'] == 'play':
        world = editor.get_game_world()
        if world:
            probes = u.GameplayStatics.get_all_actors_of_class(world, u.GunnerBlindFireProbe)
            if len(probes) == 1 and not probes[0].is_actor_tick_enabled():
                state['cycles'] += 1
                u.log(f'GUNNER_BLIND_PIE_CYCLE_FINISHED {state["cycles"]}')
                levels.editor_request_end_play()
                state.update(phase='stopping', at=time.monotonic())
                return
        if elapsed > 90:
            u.log_error('GUNNER_BLIND_PIE_TIMEOUT')
            levels.editor_request_end_play()
            finish()
    elif state['phase'] == 'stopping' and elapsed > 1 and not levels.is_in_play_in_editor():
        if state['cycles'] == 2:
            u.log('GUNNER_BLIND_PIE_COMPLETE cycles=2')
            finish()
        else:
            state.update(phase='start', at=time.monotonic())


handle = u.register_slate_post_tick_callback(tick)
