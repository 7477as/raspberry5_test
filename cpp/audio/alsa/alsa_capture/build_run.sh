#!/usr/bin/env bash
set -euo pipefail

EXE_NAME="alsa_capture"
BUILD_TYPE="${BUILD_TYPE:-Release}"
JOBS="$(nproc 2>/dev/null || echo 2)"
SRC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SRC_DIR}/build"
LOG_DIR="${BUILD_DIR}/log"
SUBCMD="${1:-run}"

case "${SUBCMD}" in
    clean)
        rm -rf "${BUILD_DIR}"
        exit 0
        ;;
    rebuild)
        rm -rf "${BUILD_DIR}"
        SUBCMD="run"
        shift || true
        ;;
    build|run)
        shift || true
        ;;
    *)
        SUBCMD="run"
        ;;
esac

require_command() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "[build_run] 缺少工具: $1"
        echo "  安装参考: sudo apt install $2"
        exit 127
    fi
}

require_command cmake cmake
require_command make build-essential
require_command pkg-config pkg-config

if ! pkg-config --exists alsa; then
    echo "[build_run] 未找到 ALSA 开发库。"
    echo "  安装参考: sudo apt install libasound2-dev"
    exit 127
fi

mkdir -p "${LOG_DIR}"
cmake -S "${SRC_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    | tee "${LOG_DIR}/configure.log"
cmake --build "${BUILD_DIR}" -j "${JOBS}" \
    | tee "${LOG_DIR}/build.log"

EXE="${BUILD_DIR}/${EXE_NAME}"
[[ -x "${EXE}" ]] || { echo "[build_run] 未找到可执行文件: ${EXE}"; exit 1; }

[[ "${SUBCMD}" == "build" || "${NORUN:-0}" == "1" ]] && exit 0

cd "${SRC_DIR}"
exec "${EXE}" "$@"
