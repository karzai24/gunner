"""Run two rendered -GunnerControllerSmoke sessions using physical gamepad keys.

The native probe traverses Enhanced Input; hardware pairing/OS delivery is not tested.
Premature PIE teardown and a missing/incomplete probe are failures, never passes.
"""
import time
import unreal as u

levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
performance = u.get_default_object(u.load_class(None, '/Script/UnrealEd.EditorPerformanceSettings'))
was_throttled = performance.get_editor_property('bThrottleCPUWhenNotForeground')
performance.set_editor_property('bThrottleCPUWhenNotForeground', False)
u.SystemLibrary.execute_console_command(editor.get_editor_world(), 't.MaxFPS 60')
state = {'phase': 'start', 'at': time.monotonic(), 'cycles': 0, 'failures': 0}


def finish():
    performance.set_editor_property('bThrottleCPUWhenNotForeground', was_throttled)
    u.unregister_slate_post_tick_callback(handle)
    u.SystemLibrary.quit_editor()


def tick(delta):
    elapsed = time.monotonic() - state['at']
    if state['phase'] == 'start' and elapsed > 3:
        levels.editor_request_begin_play()
        state.update(phase='play', at=time.monotonic(), saw_world=False)
    elif state['phase'] == 'play':
        if state.get('saw_world') and not levels.is_in_play_in_editor():
            u.log_error('GUNNER_CONTROLLER_PIE_ABORTED: play stopped before the probe completed')
            finish()
            return
        world = editor.get_game_world()
        if world:
            state['saw_world'] = True
            probes = u.GameplayStatics.get_all_actors_of_class(world, u.GunnerControllerProbe)
            if len(probes) == 1 and not probes[0].is_actor_tick_enabled():
                failures = probes[0].get_failure_count()
                state['cycles'] += 1
                state['failures'] += failures
                u.log(f'GUNNER_CONTROLLER_PIE_CYCLE_FINISHED {state["cycles"]} failures={failures}')
                levels.editor_request_end_play()
                state.update(phase='stopping', at=time.monotonic())
                return
        if elapsed > 150:
            state['failures'] += 1
            u.log_error('GUNNER_CONTROLLER_PIE_TIMEOUT')
            levels.editor_request_end_play()
            state.update(phase='failed_stopping', at=time.monotonic())
    elif state['phase'] in ('stopping', 'failed_stopping'):
        if elapsed > 10:
            u.log_error('GUNNER_CONTROLLER_PIE_STOP_TIMEOUT')
            finish()
        elif elapsed > 1 and not levels.is_in_play_in_editor():
            if state['phase'] == 'failed_stopping':
                finish()
            elif state['cycles'] == 2:
                message = f'GUNNER_CONTROLLER_PIE_COMPLETE cycles=2 failures={state["failures"]}'
                if state['failures']:
                    u.log_error(message)
                else:
                    u.log(message)
                finish()
            else:
                state.update(phase='start', at=time.monotonic())


handle = u.register_slate_post_tick_callback(tick)
