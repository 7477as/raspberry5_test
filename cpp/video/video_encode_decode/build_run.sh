#!/usr/bin/env bash
set -euo pipefail

EXE_NAME="VideoEncoder"
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
if ! pkg-config --exists gstreamer-1.0; then
    echo "[build_run] 未找到 gstreamer-1.0"
    echo "  安装参考:  sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-plugins-good gstreamer1.0-libav"
    exit 127
fi
if ! pkg-config --exists gstreamer-app-1.0; then
    echo "[build_run] 未找到 gstreamer-app-1.0"
    echo "  安装参考:  sudo apt install libgstreamer-plugins-base1.0-dev"
    exit 127
fi
if ! pkg-config --exists opencv4; then
    echo "[build_run] 未找到 opencv4"
    echo "  安装参考:  sudo apt install libopencv-dev"
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
