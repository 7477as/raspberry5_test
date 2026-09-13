#!/usr/bin/env bash
# build_run.sh - stdf_demo 构建 / 运行脚本
#
# 子命令：
#   run      默认：cmake 配置 + 编译 + 运行（需要 sudo，因为要访问 /dev/gpiochip0）
#   build    仅 cmake 配置 + 编译
#   clean    清理 build/ 目录
#   rebuild  清理后重新构建
#
# 环境变量：
#   BUILD_TYPE  Release | Debug（默认 Release）
#   NORUN=1    仅构建，不运行

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

SUBCMD="${1:-run}"
BUILD_TYPE="${BUILD_TYPE:-Release}"

require_cmd() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "[ERR] 缺少依赖：$1"
        echo "      安装：sudo apt install $2"
        exit 127
    fi
}

check_deps() {
    require_cmd cmake "cmake"
    require_cmd make "make"
    require_cmd pkg-config "pkg-config"

    if ! pkg-config --exists libgpiod; then
        echo "[WARN] 未找到 libgpiod dev 包，将以 stdout fallback 后端构建"
        echo "       Pi 真 GPIO 需安装：sudo apt install libgpiod-dev"
    fi
}

do_configure() {
    mkdir -p "${BUILD_DIR}"
    cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" \
          -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
}

do_build() {
    cmake --build "${BUILD_DIR}" -j
}

do_clean() {
    rm -rf "${BUILD_DIR}"
}

do_run() {
    local target="${BUILD_DIR}/stdf_demo"
    if [[ ! -x "${target}" ]]; then
        echo "[ERR] 未构建：${target}"
        exit 1
    fi

    if [[ ${EUID} -ne 0 ]]; then
        echo "[INFO] 重新以 sudo 启动以访问 /dev/gpiochip*"
        exec sudo "${target}"
    else
        exec "${target}"
    fi
}

check_deps

case "${SUBCMD}" in
    run)
        do_configure
        do_build
        if [[ "${NORUN:-0}" == "1" ]]; then
            echo "[OK] 仅构建，未运行（NORUN=1）"
        else
            do_run
        fi
        ;;
    build)
        do_configure
        do_build
        ;;
    clean)
        do_clean
        ;;
    rebuild)
        do_clean
        do_configure
        do_build
        ;;
    *)
        echo "用法：$0 {run|build|clean|rebuild}"
        echo "  BUILD_TYPE=Debug $0 build    # 调试构建"
        echo "  NORUN=1 $0                   # 仅构建不运行"
        exit 2
        ;;
esac
