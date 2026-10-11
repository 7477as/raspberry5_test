#!/usr/bin/env bash
set -euo pipefail

EXE_NAME="aec_webrtcdsp"
BUILD_TYPE="${BUILD_TYPE:-Release}"
JOBS="$(nproc 2>/dev/null || echo 2)"
SRC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SRC_DIR}/build"
LOG_DIR="${BUILD_DIR}/log"

SUBCMD="${1:-run}"

# 用 `shift || true` 而不是裸 shift：没有位置参数时 shift 返回 1，
# 配合 set -e 会让 `./build_run.sh` 直接静默退出
case "${SUBCMD}" in
    clean)
        rm -rf "${BUILD_DIR}"
        exit 0 ;;
    rebuild)
        rm -rf "${BUILD_DIR}"
        SUBCMD="run"
        shift || true ;;
    build)
        shift || true ;;
    run)
        shift || true ;;
    *)
        SUBCMD="run" ;;
esac

mkdir -p "${LOG_DIR}"

require_cmd() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "[build_run] 缺少工具: $1"
        echo "  安装参考:  sudo apt install $2"
        exit 127
    fi
}

require_gst_element() {
    if ! gst-inspect-1.0 "$1" >/dev/null 2>&1; then
        echo "[build_run] GStreamer 缺少元素: $1"
        echo "  安装参考:  sudo apt install $2"
        exit 127
    fi
}

require_cmd cmake cmake
require_cmd make make
require_cmd pkg-config pkg-config
require_cmd gst-inspect-1.0 gstreamer1.0-tools

for module in gstreamer-1.0 gstreamer-app-1.0 gstreamer-audio-1.0; do
    if ! pkg-config --exists "${module}"; then
        echo "[build_run] 未找到开发库: ${module}"
        echo "  安装参考:  sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev"
        exit 127
    fi
done

require_gst_element webrtcdsp gstreamer1.0-plugins-bad
require_gst_element webrtcechoprobe gstreamer1.0-plugins-bad
require_gst_element alsasrc gstreamer1.0-alsa
require_gst_element alsasink gstreamer1.0-alsa

cmake -S "${SRC_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    | tee "${LOG_DIR}/configure.log"
cmake --build "${BUILD_DIR}" -j "${JOBS}" \
    | tee "${LOG_DIR}/build.log"

EXE="${BUILD_DIR}/${EXE_NAME}"
[[ -x "${EXE}" ]] || { echo "[build_run] 未找到可执行文件: ${EXE}"; exit 1; }

[[ "${SUBCMD}" == "build" || "${NORUN:-0}" == "1" ]] && exit 0

cd "${SRC_DIR}"
exec "${EXE}" "$@"