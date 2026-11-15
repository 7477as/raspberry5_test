# BLE GATT RTT Android 客户端

> Android 连接树莓派 BLE GATT 服务，收到 Notify 后立即使用 Write Without Response 回传四时间戳测量帧。

## 功能简介

Android 为 Central/GATT Client，树莓派为 Peripheral/GATT Server；树莓派负责发送与统计，手机仅回环。
采用 Android 原生 BluetoothGatt，不依赖第三方 BLE SDK；与 SPP 版本可同时安装。
固定 20 字节帧，无分片，适合比较应用回调、系统调度和 BLE 连接事件对尾延迟的影响。

## 依赖 / 环境

- 硬件：Android 8.0/API 26+ BLE 手机、树莓派 5；无需摄像头。
- 树莓派系统：Raspberry Pi OS 64-bit，运行 `cpp/bluetooth/ble/gatts_rtt`。
- ARM64 本地构建：JDK 21、aapt2、apksigner、curl、unzip、zip。
- 标准 Gradle 构建：JDK 17、Gradle 8.11.1、Android SDK 35；无额外运行时依赖。
- 本地脚本缓存 Android SDK、Kotlin 2.0.21 和 D8；默认可复用 SPP 工程已下载的工具。

```bash
sudo apt install openjdk-21-jdk-headless aapt apksigner curl unzip zip
```

## 构建与运行

```bash
cd kotlin/bluetooth/ble/gattc_rtt
chmod +x build_apk.sh
./build_apk.sh
```

产物 `build/artifacts/gattc-debug.apk` 和 `SHA256SUMS`。支持 ARM64 树莓派本地构建；工具已缓存时无需联网。
首次无缓存需联网下载 SDK。签名密钥保存在 `build/debug.keystore`，保留密钥才能直接覆盖安装同包名版本。
标准 Android 工具链可使用 `gradle assembleDebug`；仓库提供 `.github/workflows/android-gattc.yml` 上传 APK artifact。

1. 树莓派启动 `gatts_rtt/build_run.sh run --csv build/rtt.csv`。
2. 将 APK 传到手机安装，打开 **GATT RTT Client**，授权蓝牙扫描与连接。
3. 点击扫描，选择 **RPi5_GATT_RTT**；BLE 无需预先配对。
4. 客户端完成服务发现、MTU 请求、CCCD 订阅，等待一秒稳定后发 HELLO，树莓派开始测试。
5. 保持应用前台亮屏，统计和 CSV 在树莓派端查看；下一轮须重新运行服务端并重新连接。

## 关键参数 / 配置

| 参数 | 默认值 | 说明 |
| :--- | :--- | :--- |
| 包名 | `com.raspberry.gattc` | 与 SPP 客户端独立 |
| 版本 | `1.0.0` / `1` | versionName / versionCode |
| 最低系统 | API 26 | Android 8.0 |
| 扫描 | 15 秒 | 按自定义 Service UUID 过滤 |
| 帧长度 | 20 字节 | 不依赖大 MTU |
| 写类型 | `WRITE_TYPE_NO_RESPONSE` | 回调串行处理，不插入 UI 队列 |
| 连接优先级 | `CONNECTION_PRIORITY_HIGH` | 系统请求，非保证 |
| 首选 PHY | 2M | 不支持时系统保持可用 PHY，显示回调值 |
| GATT 操作超时 | 15 秒 | 连接、初始化及写完成回调 |
| `ANDROID_TOOL_CACHE` | SPP 工程工具目录 | 可指定已有构建工具缓存 |

Service UUID 为 `d3b5f100-6172-4c29-a563-45b9c8e57001`；写和通知特征分别为 `f101`、`f102`，其余 UUID 字段相同。
请求格式为 4 字节会话序号、8 字节树莓派 t1、8 字节 GATTRTT1 标记；回包为相同序号、手机 t2、手机 t3，全部大端。
手机时间戳为 `elapsedRealtimeNanos()`；树莓派保存 t1，在收到回写时记录 t4。

## 已知问题 / 注意事项

- Android 12+ 需要 BLUETOOTH_SCAN/CONNECT；Android 8～11 需要位置权限及系统定位开关。
- Android 14+ 首次 MTU 请求可能由系统按 517 发起，显示实际协商值，数据帧仍固定 20 字节。
- 四时间戳中的往返分解属于校准估计，依赖双向对称和时钟等速假设；RTT 和手机处理才是同钟实测。
- GATT 写尚未完成时若收到下一帧，明确报告 busy 并断开；先用单在途基线，高频窗口不保证所有手机可用。
- Notify 和无响应写没有 ATT 层确认；BLE 链路仍有重传，程序不做应用层重传。
- 未实现配对/加密，适合本地实验；不要作为公开生产服务。
- PHY 显示的是成功回调结果；高连接优先级和 2M 仅是请求，树莓派连接间隔可用 btmon 观察。
- 调试 APK 可安装运行，手机真实延迟、权限流程和不同厂商兼容性仍需实机验证。

## 参考资料

- [Android BluetoothGatt](https://developer.android.com/reference/android/bluetooth/BluetoothGatt)
- [BlueZ GATT API](https://bluez.readthedocs.io/en/latest/gatt-api/)
- [树莓派服务端](../../../../cpp/bluetooth/ble/gatts_rtt/README.md)
