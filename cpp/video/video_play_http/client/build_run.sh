#!/usr/bin/env bash
set -euo pipefail

EXE_NAME="HttpVideoPlayer"
BUILD_TYPE="${BUILD_TYPE:-Release}"
JOBS="$(nproc 2>/dev/null || echo 2)"
SRC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SRC_DIR}/build"
LOG_DIR="${BUILD_DIR}/log"

SUBCMD="${1:-run}"
case "${SUBCMD}" in
    clean)
        rm -rf "${BUILD_DIR}"
        exit 0 ;;
    rebuild)
        rm -rf "${BUILD_DIR}" ;;
    run|build|"") ;;
    *)
        SUBCMD="run"
        set -- "$@" ;;
esac

mkdir -p "${LOG_DIR}"

require_cmd() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "[build_run] 缺少工具: $1"
        echo "  安装参考:  sudo apt install $2"
        exit 127
    fi
}
require_cmd cmake cmake
require_cmd make make

# 依赖检测
if ! pkg-config --exists libavcodec libavutil libswresample; then
    echo "[build_run] 未找到 FFmpeg (libavcodec/libavutil/libswresample)"
    echo "  安装参考:  sudo apt install libavcodec-dev libavutil-dev libswresample-dev"
    exit 127
fi
if ! pkg-config --exists sdl2; then
    echo "[build_run] 未找到 SDL2"
    echo "  安装参考:  sudo apt install libsdl2-dev"
    exit 127
fi
if ! pkg-config --exists libcurl; then
    echo "[build_run] 未找到 libcurl"
    echo "  安装参考:  sudo apt install libcurl4-openssl-dev"
    exit 127
fi

cmake -S "${SRC_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    | tee "${LOG_DIR}/configure.log"
cmake --build "${BUILD_DIR}" -j "${JOBS}" \
    | tee "${LOG_DIR}/build.log"

EXE="${BUILD_DIR}/${EXE_NAME}"
[[ -x "${EXE}" ]] || { echo "[build_run] 未找到可执行文件: ${EXE}"; exit 1; }

[[ "${SUBCMD}" == "build" || "${NORUN:-0}" == "1" ]] && exit 0

cd "${SRC_DIR}"
exec "${EXE}" "$@"
