# Blender Asset Pipeline

## Installed toolchain

- Blender 4.5.10 LTS, installed from the Blender Foundation Windows package (`BlenderFoundation.Blender.LTS.4.5`).
- BlendMCP 1.4.3, an MIT-licensed telemetry-free fork of BlenderMCP.
- BlendMCP's matching Blender add-on is installed and enabled for Blender 4.5.
- Codex launches the MCP server from `C:\Users\karza\.local\bin\blendmcp.exe` and connects to Blender on `127.0.0.1:9876`.
- Poly Haven, Hyper3D, Sketchfab, and Hunyuan3D integrations are disabled by default. Enable one only for a deliberate, provenance-reviewed import.

The bridge can execute Blender Python with the permissions of the current Windows user. Save source files before large procedural edits and use dedicated structured tools when they cover the operation.

## Connect Codex to Blender

1. Open Blender 4.5 LTS.
2. In the 3D Viewport, press `N`, open the `BlendMCP` tab, and select `Connect to Claude`. The label is inherited from the add-on; it starts the same local bridge Codex uses.
3. Start or restart a Codex task after MCP configuration changes so the Blender tools are loaded.
4. Ask Codex to call `get_blender_status` before editing. A healthy response reports `connected: true` and matching server/add-on versions.

Only one Blender instance should listen on port `9876` at a time.

## Source and export rules

- Use 1 Blender unit = 1 meter and keep scene units metric.
- Author sources under `assets/source/`, grouped by asset family when the library grows.
- Apply object scale and rotation before final export unless animation or rigging requires otherwise.
- Use clear `snake_case` object, material, action, and file names. Avoid Blender-generated names such as `Cube.001` in approved content.
- Put origins and pivots at deliberate gameplay locations: feet for characters, grip or mount points for weapons, and grid-snapped bases for modular environment pieces.
- Export glTF 2.0 binary (`.glb`) into `assets/characters/`, `assets/environments/`, `assets/weapons/`, or another appropriate runtime folder. Blender's glTF exporter handles Blender Z-up to Godot Y-up conversion.
- Validate scale, orientation, materials, animation names, collisions, and import warnings in Godot before treating an asset as complete.
- Record any externally sourced or generated input in `docs/ASSET_PROVENANCE.md` before integration.

## Repair or update

The package currently needs the MCP Python 1.x API. Keep the compatibility pin until BlendMCP declares MCP 2.x support:

```powershell
$env:UV_PYTHON_PREFERENCE = "only-managed"
uv tool install "blendmcp==1.4.3" --with "mcp<2" --python 3.11 --force
C:\Users\karza\.local\bin\blendmcp.exe install-addon --blender-version 4.5
```

After updating, restart Blender and Codex, then verify `get_blender_status`, `get_scene_info`, and a reversible create/delete operation.
