# Realtime Thread

125μs tight-loop jitter benchmark on Linux. Demonstrates CPU isolation + SCHED_FIFO + mlockall.

## Layout

```
realtime_thread/
├── CMakeLists.txt
├── build_run.sh
├── Main.cpp             ← entry
├── RealtimeThread.cpp/.h
└── README.md
```

## Prerequisites

```bash
sudo apt install cmake build-essential
```

### CPU isolation (mandatory)

1. Edit `/boot/firmware/cmdline.txt`:

   ```bash
   sudo nano /boot/firmware/cmdline.txt
   ```

2. Append to the end of the existing line (space-separated):

   ```
   isolcpus=3 nohz_full=3 rcu_nocbs=3
   ```

3. Save and reboot:

   ```bash
   sudo reboot
   ```

## Build & run

```bash
chmod +x build_run.sh
./build_run.sh          # configures + builds + runs (auto-sudo)
./build_run.sh build
./build_run.sh clean
./build_run.sh rebuild
NORUN=1 ./build_run.sh
```

The script automatically escalates to `sudo` (SCHED_FIFO / mlockall require root).

Example output:

```
[-] 成功将线程强制绑定到 CPU 核心 3
[-] 成功开启 SCHED_FIFO 实时调度，最高优先级
[-] 成功锁定内存，防止 Swap 交换

开始测试，目标间隔: 125000 ns (125 us)...

--- 125μs 循环测试结果 (共 800000 次) ---
最大误差 (Max Jitter): ...
最小误差 (Min Jitter): ...
平均误差 (Avg Jitter): ...
```

## Parameters

Edit `Main.cpp`'s `rt.Run(...)` call (uses `RealtimeThread::Config`):

| Field | Default | Meaning |
| :--- | :--- | :--- |
| `targetCore` | `3` | CPU to bind to (must match `isolcpus`) |
| `intervalNs` | `125000` | Target interval (ns) |
| `testIterations` | `800000` | Loop count (~1 second) |

Example: 250μs loop on CPU 3 for ~1 second:

```cpp
RealtimeThread::Config cfg;
cfg.targetCore     = 3;
cfg.intervalNs     = 250000;
cfg.testIterations = 400000;
rt.Run(cfg);
```

## Notes

- Requires `isolcpus=3` in kernel cmdline; otherwise the kernel may move the task off the isolated core.
- On Pi 5, 125μs is generally not achievable — typical interrupt latency is >50μs. Try 250μs or longer.
- `mlockall` may fail if the process exceeds the locked-memory limit; run as root.
- This is a single-process busy-loop benchmark; it does not represent a complex workload.
