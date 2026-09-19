#!/bin/bash
set -euo pipefail
project_root="$(cd "$(dirname "$0")/.." && pwd)"
engine_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
target="${1:-GunnerEditor}"
"$engine_root/Engine/Build/BatchFiles/Mac/Build.sh" "$target" Mac Development "$project_root/Gunner.uproject" -WaitMutex -NoHotReloadFromIDE
