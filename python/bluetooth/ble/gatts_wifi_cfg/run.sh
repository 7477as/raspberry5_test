#!/usr/bin/env bash
set -euo pipefail

source /home/moyuping/myenv/bin/activate

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"

if ! command -v python3 >/dev/null 2>&1; then
    echo "错误：未找到 python3，请先安装 Python 3。" >&2
    exit 127
fi

if [[ -x "${SCRIPT_DIR}/.venv/bin/python3" ]]; then
    PYTHON_BIN="${SCRIPT_DIR}/.venv/bin/python3"
else
    PYTHON_BIN="$(command -v python3)"
fi

if [[ ! -f main.py ]]; then
    echo "错误：找不到 ${SCRIPT_DIR}/main.py。" >&2
    exit 1
fi

if [[ "${EUID}" -ne 0 ]]; then
    echo "提示：BLE GATT 服务通常需要 root 权限，正在请求 sudo。"
    exec sudo -E "${PYTHON_BIN}" main.py "$@"
fi

exec "${PYTHON_BIN}" main.py "$@"
