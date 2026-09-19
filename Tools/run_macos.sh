#!/bin/bash
set -euo pipefail
project_root="$(cd "$(dirname "$0")/.." && pwd)"
engine_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
exec "$engine_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" "$project_root/Gunner.uproject" -game -windowed -ResX=1280 -ResY=720 "$@"
