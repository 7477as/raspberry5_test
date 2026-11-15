#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
BUILD_TYPE="${BUILD_TYPE:-Release}"
TARGET_BIN="${BUILD_DIR}/stdf_mvc_demo"
cmd="${1:-run}"

case "${cmd}" in
    clean)
        rm -rf "${BUILD_DIR}"
        exit 0
        ;;
    rebuild)
        rm -rf "${BUILD_DIR}"
        cmd=build
        ;;
    run|build) ;;
    *)
        echo "Usage: $0 {run|build|clean|rebuild}" >&2
        exit 1
        ;;
esac

for tool in cmake gcc pkg-config; do
    if ! command -v "${tool}" >/dev/null 2>&1; then
        echo "[ERROR] ${tool} 未安装，请先: sudo apt install cmake gcc pkg-config" >&2
        exit 127
    fi
done
if ! pkg-config --exists sdl2; then
    echo "[WARN] SDL2 UI 将跳过；如需启用: sudo apt install libsdl2-dev" >&2
fi
cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
cmake --build "${BUILD_DIR}" -j"$(nproc)"
if [[ "${cmd}" == "run" && "${NORUN:-0}" != "1" ]]; then
    exec "${TARGET_BIN}"
fi
