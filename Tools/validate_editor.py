"""Run with -GunnerSmoke -ExecCmds="py <this file>" in the full editor.
Exercises two complete PIE lifecycles; logs GUNNER_EDITOR_CYCLES_COMPLETE.
"""
import time
import unreal as u
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
state = {'phase':'start', 'at':time.monotonic(), 'cycles':0}

def tick(delta):
    elapsed=time.monotonic()-state['at']
    if state['phase']=='start' and elapsed>5:
        levels.editor_request_begin_play()
        state.update(phase='play',at=time.monotonic())
    elif state['phase']=='play':
        world=editor.get_game_world()
        if world:
            probes=u.GameplayStatics.get_all_actors_of_class(world,u.GunnerFoundationProbe)
            if len(probes)==1 and not probes[0].is_actor_tick_enabled():
                u.log(f'GUNNER_EDITOR_CYCLE_FINISHED {state["cycles"]+1}')
                levels.editor_request_end_play()
                state.update(phase='stopping',at=time.monotonic())
                return
        if elapsed>90:
            u.log_error('GUNNER_EDITOR_CYCLE_TIMEOUT')
            u.unregister_slate_post_tick_callback(handle)
            levels.editor_request_end_play()
    elif state['phase']=='stopping' and elapsed>3:
        if levels.is_in_play_in_editor():
            u.log_error('GUNNER_EDITOR_FAILED_TO_STOP')
            u.unregister_slate_post_tick_callback(handle)
            return
        state['cycles']+=1
        if state['cycles']==2:
            u.log('GUNNER_EDITOR_CYCLES_COMPLETE')
            u.unregister_slate_post_tick_callback(handle)
            u.SystemLibrary.quit_editor()
        else:
            state.update(phase='start',at=time.monotonic())

handle=u.register_slate_post_tick_callback(tick)
