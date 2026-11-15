# BLE GATT RTT 服务端

> 树莓派发送 BLE Notify，Android 使用无响应写回环，测量 RTT、手机处理耗时和往返估计。

## 功能简介

本工程在树莓派上作为 GATT Server/Peripheral，Android 作为 GATT Client/Central。
树莓派发通知，手机立即回写；树莓派保存全部统计和 CSV，适合与经典蓝牙 SPP 进行应用层延迟对比。
与直接 HCI 测量相比，本方案包含 BlueZ D-Bus、系统排队和 Android 回调调度开销，不代表无线空口时延。

目录包含 `Main.cpp`、`GattServer.*`、`PacketCodec.h`、`TimingEstimate.h`、`LatencyBenchmark.*` 和 `BenchmarkTest.cpp`。

## 依赖 / 环境

- 硬件：树莓派 5 内置 BLE，Android 8.0/API 26 或以上手机；无需摄像头。
- 系统：Raspberry Pi OS 64-bit；本机 BlueZ 系统总线验证通过。
- 构建：CMake 3.16+、C++17、GLib/GIO、BlueZ。

```bash
sudo apt install build-essential cmake pkg-config libglib2.0-dev bluez
sudo systemctl start bluetooth
bluetoothctl power on
```

## 构建与运行

先在树莓派启动，再在 Android 安装并打开 `kotlin/bluetooth/ble/gattc_rtt/build/artifacts/gattc-debug.apk`，扫描并连接 `RPi5_GATT_RTT`。
无需预先配对。只有 Notify 订阅及 HELLO 完成后才开始计时，Android 停止扫描并请求高连接优先级与 2M PHY。

```bash
cd cpp/bluetooth/ble/gatts_rtt
chmod +x build_run.sh
./build_run.sh run --count 1000 --warmup 100 --csv build/rtt.csv
./build_run.sh build
./build_run.sh clean
./build_run.sh rebuild
NORUN=1 ./build_run.sh
BUILD_TYPE=Debug ./build_run.sh build
ctest --test-dir build --output-on-failure
```

固定频率模式使用有界窗口，窗口满时跳过时隙，不补发积压包：

```bash
./build_run.sh run --fixed-rate --interval-ms 10 --window 1 --csv build/rtt-10ms.csv
./build_run.sh run --fixed-rate --interval-ms 5 --window 2 --csv build/rtt-5ms.csv
```

若出现 Android `GATT busy`，降低发送频率或使用单在途基线；首版遇到 busy 会明确断开，避免隐藏排队造成误导。
遇到 D-Bus 权限拒绝，先检查系统 BlueZ policy；必要时用 `sudo ./build/gatts` 运行已构建程序。脚本不更改系统服务配置、不自动安装软件。

## 关键参数 / 配置

| 参数 | 默认值 | 说明 |
| :--- | :--- | :--- |
| `--adapter` | `hci0` | BlueZ 适配器 |
| `--count` | `1000` | 正式测量数 |
| `--warmup` | `100` | 预热与校准帧数 |
| `--interval-ms` | `20` | 发送间隔，正整数 |
| `--timeout-ms` | `1000` | 单帧超时 |
| `--window` | `1` | 最大在途帧，1～32；大于 1 需固定频率 |
| `--fixed-rate` | 关闭 | 固定时隙；默认等回包后再发送 |
| `--wait-seconds` | `120` | 等待手机就绪期限 |
| `--csv` | 不写文件 | 输出路径，父目录必须存在 |
| `BUILD_TYPE` / `NORUN` | `Release` / `0` | 构建模式 / 仅构建 |
| Service UUID | `d3b5f100-6172-4c29-a563-45b9c8e57001` | 自定义 128 位 UUID |
| RX UUID | `d3b5f101-6172-4c29-a563-45b9c8e57001` | Android Write Without Response |
| TX UUID | `d3b5f102-6172-4c29-a563-45b9c8e57001` | 树莓派 Notify |

所有消息恰好 20 字节，大端，不分片，默认 ATT MTU=23 即可。
请求：4 字节 token + 8 字节 t1 + 8 字节魔数 `0x4741545452545431`（GATTRTT1）。
响应：4 字节相同 token + 8 字节 Android t2 + 8 字节 Android t3。
HELLO：token=`0xffffffff`，第二字段为手机随机会话前缀，第三字段为相同魔数。
token 高 12 位是本次会话前缀，低 20 位是序号；单次预热和测量总数须小于 1048575。

`t1` 在发 Notify 前、`t4` 在接收回写回调入口记录，均为树莓派单调时钟；`t2` 是 Android 通知回调入口，`t3` 在准备调用写 API 前记录，使用 `elapsedRealtimeNanos()`。
手机处理为 `t3-t2`，净 RTT 为 `(t4-t1)-(t3-t2)`；Android API 调用及写队列耗时计入返程。

校准使用预热样本中净 RTT 最小的一帧，`offset=((t2-t1)+(t3-t4))/2`。
往程为 `t2-t1-offset`，返程为 `t4-t3+offset`；两者之和等于净 RTT。
`--warmup 0` 时用全部有效样本；预热全部失败时只输出 RTT 和处理耗时，不输出单向估计。

输出 RTT/处理耗时/净 RTT/往返估计的 min、mean、P50、P95、P99、max，以及超时率、异常、迟到和跳过时隙数。
CSV 保存 t1～t4、状态、各项耗时、校准偏差、校准误差界和距校准时刻秒数。
校准误差界为最小净 RTT 的一半，仅在等速时钟和非负路径假设下适用，未计漂移及时间戳误差。

## 已知问题 / 注意事项

- 单向结果标记 ESTIMATE，依赖校准帧双向对称，不是严格单向实测；保留负值，不校正时钟漂移，建议短测试并保持两端不休眠。
- 使用标准 BlueZ `WriteValue` / `PropertiesChanged` 数据路径，包含每包 D-Bus 开销；未实现可选 AcquireWrite/AcquireNotify FD 优化。
- 一次只允许一个手机连接测试。完成后程序退出；重新测试须重新启动程序并在手机重新连接。
- 20 字节回包没有额外 CRC 或 t1 回显，靠会话 token、序号、长度、时间戳合理性及 BLE 链路校验检测错误；不适合不可信对端测试。
- 2M PHY 与高连接优先级属于请求，不保证成功。手机显示 PHY 回调实测值；树莓派无法从标准 GATT D-Bus 获取 PHY/连接间隔，标记 unknown，不虚报数值。
- Android 14+ 即使请求 MTU 23，系统仍可能协商更大 MTU；帧始终保持 20 字节。
- 协议没有应用层重传，超时不等于底层无线丢包。BLE 链路本身仍有重传与流控；Notify/无响应写没有 ATT 响应。
- 不包含加密或认证；用于本地实验。连接间隔、PHY、Android 省电和无线调度均会影响尾延迟，高优先级增加功耗。
- 性能比较请记录手机型号、系统、PHY 和 MTU；可用 `sudo btmon` 观察连接间隔、PHY 更新和断开原因。
- 本地模拟与 GATT/广播注册验证不等于真实手机 RTT 验证；实机需分别测 5/10/20 ms 并与 SPP 比较。

## 参考资料

- [BlueZ GATT API](https://bluez.readthedocs.io/en/latest/gatt-api/)
- [Android BluetoothGatt](https://developer.android.com/reference/android/bluetooth/BluetoothGatt)
- [Android 客户端](../../../../kotlin/bluetooth/ble/gattc_rtt/README.md)
