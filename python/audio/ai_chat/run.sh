#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

VENV_ACTIVATE="/home/moyuping/myenv/bin/activate"

usage() {
    cat <<'EOF'
用法: ./run.sh [run|list|clean]

  run    启动 AI 语音助手（默认）
  list   列出麦克风 / 扬声器声卡（arecord -l / aplay -l）
  clean  删除调试录音与 __pycache__
EOF
}

list_devices() {
    echo "=== 麦克风 (arecord -l) ==="
    arecord -l || echo "未找到 arecord，请先安装 alsa-utils"
    echo ""
    echo "=== 扬声器 (aplay -l) ==="
    aplay -l || echo "未找到 aplay，请先安装 alsa-utils"
    echo ""
    echo "把看到的 card/device 填进 config.py："
    echo "  MIC_DEVICE=\"plughw:<麦克风card>,<device>\""
    echo "  SPEAKER_DEVICE=\"plughw:<扬声器card>,<device>\"（mpv 用 alsa/plughw:<card>,<device>）"
}

do_clean() {
    rm -f debug_recording.wav
    rm -rf __pycache__
    echo "已清理。"
}

do_run() {
    if [[ ! -f "$VENV_ACTIVATE" ]]; then
        echo "❌ 找不到虚拟环境: $VENV_ACTIVATE" >&2
        echo "请先创建: python3 -m venv /home/moyuping/myenv" >&2
        exit 1
    fi
    # shellcheck disable=SC1090
    source "$VENV_ACTIVATE"

    command -v python3 >/dev/null || { echo "❌ 找不到 python3"; exit 127; }
    python3 -c "import gi" 2>/dev/null || {
        echo "❌ 缺少 PyGObject/GStreamer，请执行:" >&2
        echo "   sudo apt install python3-gi gir1.2-gstreamer-1.0 gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-alsa" >&2
        exit 127
    }
    command -v mpv >/dev/null || {
        echo "❌ 未找到 mpv，请执行: sudo apt install mpv" >&2
        exit 127
    }
    command -v ollama >/dev/null || echo "⚠️ 未找到 ollama，LLM 问答会失败，请先安装并 ollama pull qwen2:1.5b"

    exec python3 main.py "$@"
}

CMD="${1:-run}"
if [[ $# -gt 0 ]]; then shift; fi
case "$CMD" in
    run) do_run "$@" ;;
    list) list_devices ;;
    clean) do_clean ;;
    -h|--help|help) usage ;;
    *) echo "未知子命令: $CMD"; usage; exit 1 ;;
esac
