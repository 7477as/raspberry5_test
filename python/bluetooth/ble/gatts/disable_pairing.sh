#!/bin/bash
# 手动禁用蓝牙配对的辅助脚本

set -euo pipefail

echo "=========================================="
echo "禁用树莓派蓝牙配对模式"
echo "=========================================="
echo ""

echo "步骤 1: 使用 bluetoothctl 禁用配对"
echo "执行命令: bluetoothctl pairable off"
bluetoothctl pairable off

echo ""
echo "步骤 2: 验证配置"
echo "执行命令: bluetoothctl show | grep Pairable"
bluetoothctl show | grep Pairable

echo ""
echo "✅ 配对模式已禁用"
echo ""
echo "如需恢复配对功能，执行:"
echo "  bluetoothctl pairable on"
echo ""
