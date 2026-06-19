# 微秒级实时任务运行指南

本项目包含三个文件：内核配置文件说明、核心 C++ 代码、以及一键运行脚本。

## 1. 隔离核心 (修改内核参数)

必须修改系统内核启动参数，隔离 CPU 3 以防止操作系统干扰。

1. 打开配置文件：
   ```bash
   sudo nano /boot/firmware/cmdline.txt 
   ```

2. 在文件**末尾**追加以下参数（必须与原有内容保持在同一行，用空格隔开）：
   ```text
   isolcpus=3 nohz_full=3 rcu_nocbs=3
   ```

3. 保存并退出 (`Ctrl+O`, `Enter`, `Ctrl+X`)。

4. **必须重启系统**使配置生效：
   ```bash
   sudo reboot
   ```