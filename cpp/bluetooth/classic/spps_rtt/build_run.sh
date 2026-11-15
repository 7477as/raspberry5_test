#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
action="${1:-run}"
if (($#)); then shift; fi
if [[ "$action" == clean || "$action" == rebuild ]]; then
    cmake -E rm -rf "$project_dir/build"
    [[ "$action" == clean ]] && exit 0
fi
case "$action" in run|build|rebuild) ;; *) echo "Usage: $0 [run|build|clean|rebuild] [spps options]" >&2; exit 2;; esac
for tool in cmake c++ pkg-config; do
    if ! command -v "$tool" >/dev/null; then
        echo "Missing $tool; sudo apt install build-essential cmake pkg-config libglib2.0-dev bluez" >&2
        exit 127
    fi
done
if ! pkg-config --exists gio-unix-2.0; then
    echo "Missing GIO; sudo apt install libglib2.0-dev" >&2
    exit 127
fi
if [[ -f "$project_dir/build/CMakeCache.txt" ]]; then
    cached_source="$(while IFS= read -r line; do
        if [[ "$line" == CMAKE_HOME_DIRECTORY:INTERNAL=* ]]; then
            printf '%s' "${line#*=}"
            break
        fi
    done < "$project_dir/build/CMakeCache.txt")"
    if [[ "$cached_source" != "$project_dir" ]]; then
        cmake -E rm -f "$project_dir/build/CMakeCache.txt"
        cmake -E rm -rf "$project_dir/build/CMakeFiles"
    fi
fi
cmake -S "$project_dir" -B "$project_dir/build" -DCMAKE_BUILD_TYPE="${BUILD_TYPE:-Release}"
cmake --build "$project_dir/build" --parallel "${JOBS:-2}"
if [[ "$action" != build && "${NORUN:-0}" != 1 ]]; then
    exec "$project_dir/build/spps" "$@"
fi
