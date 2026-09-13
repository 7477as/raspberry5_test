#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
BUILD_TYPE="${BUILD_TYPE:-Release}"
TARGET_BIN="${BUILD_DIR}/stdf_mvc_demo"

cmd="${1:-run}"

if ! command -v cmake >/dev/null 2>&1; then
    echo "[ERROR] cmake 未安装，请先: sudo apt install cmake" >&2
    exit 127
fi

if ! command -v gcc >/dev/null 2>&1; then
    echo "[ERROR] gcc 未安装，请先: sudo apt install gcc" >&2
    exit 127
fi

case "${cmd}" in
    build)
        cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" \
            -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" >/dev/null
        cmake --build "${BUILD_DIR}" -j"$(nproc)"
        echo "[OK] build -> ${TARGET_BIN}"
        ;;
    clean)
        rm -rf "${BUILD_DIR}"
        echo "[OK] clean -> ${BUILD_DIR}"
        ;;
    rebuild)
        rm -rf "${BUILD_DIR}"
        exec "${BASH_SOURCE[0]}" build
        ;;
    run|"")
        if [[ ! -x "${TARGET_BIN}" || "${NORUN:-0}" != "1" ]]; then
            cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" \
                -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" >/dev/null
            cmake --build "${BUILD_DIR}" -j"$(nproc)"
        fi
        echo "[OK] run -> ${TARGET_BIN}"
        exec "${TARGET_BIN}"
        ;;
    *)
        echo "Usage: $0 {run|build|clean|rebuild}" >&2
        exit 1
        ;;
esac
