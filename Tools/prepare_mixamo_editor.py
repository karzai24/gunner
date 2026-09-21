"""Import and retarget the local Mixamo manifest in the normal editor.

Use -ExecCmds="py <absolute-path>/Tools/prepare_mixamo_editor.py".
The UE 5.8 FBX mesh importer needs Slate; do not use -run=pythonscript.
The editor exits after success or a reported failure. Launch in a clean editor
process, with no unsaved user editing session open.
"""
from pathlib import Path
import runpy
import unreal as u

try:
    root = Path(u.Paths.project_dir()).resolve()
    for name in ('import_mixamo_source', 'retarget_mixamo'):
        runpy.run_path(str(root / 'Tools' / (name + '.py')), run_name='__main__')
finally:
    u.SystemLibrary.quit_editor()
