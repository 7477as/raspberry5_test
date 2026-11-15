# SPP 低延迟 RTT 服务端

> 树莓派通过经典蓝牙 SPP 向 Android 发送测试帧，测量应用层往返延迟。

## 功能简介

BlueZ D-Bus 注册标准 SPP 服务，Android 客户端连接后立即回传完整帧，服务端使用同一个单调时钟计算 RTT。
相比虚拟串口透传，直接操作 RFCOMM socket，提供流式拆包、超时、负载调度及 CSV；相比 BLE，适合已采用经典蓝牙 SPP 的 Android 设备。
支持单在途基线和固定频率负载测试，输出 min、mean、P50、P95、P99、max 及超时率。

## 依赖 / 环境

- 硬件：树莓派 5 内置经典蓝牙；Android 8.0 及以上、支持经典蓝牙的手机。无需摄像头。
- 系统：Raspberry Pi OS / Debian ARM64，BlueZ 5，CMake 3.16+，C++17。
- Android 客户端：[sppc](../../../../kotlin/bluetooth/classic/sppc_rtt/README.md)。

```bash
sudo apt install build-essential cmake pkg-config libglib2.0-dev bluez
sudo systemctl start bluetooth
bluetoothctl power on
```

## 构建与运行

```bash
cd cpp/bluetooth/classic/spps_rtt
chmod +x build_run.sh
./build_run.sh build
./build_run.sh # 配置、构建并等待手机连接
./build_run.sh run --count 1000 --csv build/rtt.csv
./build_run.sh clean
./build_run.sh rebuild
NORUN=1 ./build_run.sh
BUILD_TYPE=Debug ./build_run.sh build
```

1. 安装手机 APK，先在系统蓝牙设置中配对树莓派。首次配对可在树莓派终端启用 agent，并保持终端运行：

```bash
bluetoothctl
agent on
default-agent
pairable on
discoverable on
```

2. 在双方确认配对提示。如果需要手动信任手机：

```bash
bluetoothctl trust <手机MAC>
```

3. 启动服务端，看到 `SPP service RPi5_SPP_RTT registered` 后，在手机 App 刷新、选择树莓派、连接。测试自动开始，完成后打印统计并断开。
4. 每次测试重新运行服务端，手机重新连接。进行负载测试：

```bash
./build_run.sh run --mode fixed --window 8 --interval-ms 5 --size 64 --csv build/load.csv
ctest --test-dir build --output-on-failure
```

一般无需 root；若系统 D-Bus 策略拒绝注册，检查本机 BlueZ 策略和用户权限。无需启用 bluetoothd 的 legacy compatibility 模式。

## 关键参数 / 配置

| 参数 / 环境变量 | 默认值 | 说明 |
| :--- | :--- | :--- |
| `--mode` | `baseline` | `baseline` 单在途；`fixed` 固定频率 |
| `--size` | `64` | 整帧长度，包含 40 字节头，范围 40～65536 |
| `--count` | `1000` | 正式测量发包数 |
| `--warmup` | `20` | 预热包数；预热与正式样本合计不超过 1000000 |
| `--interval-ms` | `20` | 发包目标间隔，正整数毫秒 |
| `--timeout-ms` | `1000` | 从发送前计时开始计算超时，正整数毫秒 |
| `--window` | `1` | 固定频率模式在途上限 1～1040；基线强制为 1 |
| `--csv` | 不导出 | 结束后写入 CSV，目标父目录需已存在 |
| `BUILD_TYPE` | `Release` | CMake 构建类型 |
| `NORUN` | `0` | `1` 时只构建 |
| `JOBS` | `2` | 编译并行数 |

协议多字节字段均为大端：偏移 0 为 4 字节魔数 `SPP1`，4 为 1 字节版本 `2`，5 为 1 字节类型 `1`，6 为 2 字节头长度 `40`，8 为 4 字节总长度，12 为 4 字节序号，16 为 8 字节树莓派发送时间戳 t1（ns），24 为 8 字节 Android 收齐帧时间 t2，32 为 8 字节 Android 回传前时间 t3。树莓派收齐回包记录 t4。
40 字节以后的每个字节为 `(sequence + offset) & 255`，客户端填写 t2/t3 后回传，服务端校验序号、t1、长度和负载内容。协议 v2 必须配套 Android 1.1.0 或以上，旧版 APK 不兼容。

RTT 从首次尝试发送该帧前开始，到包含帧末字节的 `recv` 返回后结束，包含时间戳填写、系统排队、无线传输、手机回环及调度开销。
分位数使用成功正式样本的 nearest-rank 算法，预热和超时样本不参与分位数。CSV 保留预热和超时状态。

四时间戳额外输出实测手机处理耗时 `t3-t2`，以及扣除该耗时的 RTT `(t4-t1)-(t3-t2)`。t2 在手机读齐帧后记录，t3 在调用 write 前记录，手机写阻塞属于返程路径。

校准只使用成功预热样本中扣除手机处理后 RTT 最小的一帧，估计 Android 相对树莓派偏差 `offset=((t2-t1)+(t3-t4))/2`；`--warmup 0` 时改用全部成功样本。预热全超时则不输出单向估计。
往程估计为 `t2-t1-offset`，返程估计为 `t4-t3+offset`；同一帧两者之和等于扣除手机处理后的 RTT。

输出中的 `ESTIMATE` 依赖校准帧双向对称假设，不是精确单向实测。`offset_bound_at_calibration_ms` 为非负路径和等速时钟假设下，校准帧扣除处理后 RTT 的一半；不包含时间戳误差和后续漂移。
CSV 包含手机 t2/t3、处理耗时、净 RTT、往返估计、偏差、校准界及样本距校准帧时间；不截断负估计。长时间测试需分段重跑，本实现不校正时钟频率漂移，设备测试期间不能休眠。

## 已知问题 / 注意事项

- `RTT / 2` 仅为双向对称假设下的估计，不是实测单向延迟；本工具不执行 PTP 时钟同步。
- SPP 无法保证固定毫秒上限。手机保持 App 前台亮屏，避免配对扫描、其他蓝牙流量及 2.4 GHz 干扰，记录手机型号和系统版本。
- 固定频率模式在窗口满或调度落后时跳过发送时隙，`skipped_slots` 不是无线丢包；`--count` 指实际安排的包数，不含跳过时隙。
- 超时是未及时收到有效应用回包，不代表底层无线丢包；迟到回包单独计数。结束后未继续观察的迟到回包不会计入。
- `send_rate_hz` 按所有发送样本（含预热）除以总运行时长计算，包含最后等待回包的时间。
- 单次只接受一个客户端，一次测试后退出。连接前耗时不计入 RTT。断连或无效协议头会终止测试。
- 已开始但未完成的帧写超时会终止连接，避免破坏字节流。成功退出码为 0，全部正式包超时为 2，协议或 I/O 错误为 1。
- 模拟回环测试不代表真实蓝牙性能；需使用手机对比 48/64/256 字节及 20/10/5 ms 间隔。

## 参考资料

- [BlueZ Profile API](https://github.com/bluez/bluez/blob/master/doc/org.bluez.ProfileManager.rst)
- [Android 蓝牙数据传输](https://developer.android.com/develop/connectivity/bluetooth/transfer-data)
- [Android BluetoothSocket](https://developer.android.com/reference/android/bluetooth/BluetoothSocket)
