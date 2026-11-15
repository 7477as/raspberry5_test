# Android SPP RTT 回环客户端

> 安装 APK 后连接树莓派，收到完整二进制测试帧，填写手机收发时间戳后立即回传。

## 功能简介

精简 Kotlin 原生 App，使用系统 `BluetoothSocket` 和传统 View，无 Compose 或第三方蓝牙框架。
与通用蓝牙串口终端相比，内置本项目的二进制分帧和自动回环，无需手动输入数据，适用于 C++ 服务端 RTT 测试。
数据收发在专用线程执行，UI 计数最多每 500 ms 更新一次。

## 依赖 / 环境

- 手机：Android 8.0（API 26）及以上，支持经典蓝牙 SPP。目标 Android API 35。
- 服务端：树莓派 5，[C++ spps](../../../../cpp/bluetooth/classic/spps_rtt/README.md)。无需摄像头。
- 使用 APK 无需 Android Studio、JDK、SDK 或开发者账号。
- 本地重新构建才需要 JDK 21、Debian ARM64 `aapt`、`apksigner` 及网络连接；也提供 x86_64 GitHub Actions Gradle 构建配置。

## 构建与运行

### 安装现成 APK

构建产物在 `build/artifacts/sppc-debug.apk`，从远程工作区下载到手机后安装。APK 不提交到 Git。
仅在系统提示时，为用于打开 APK 的文件管理器或浏览器允许“安装未知应用”。

1. 在手机系统蓝牙设置中配对树莓派，双方确认提示。
2. 启动树莓派 `spps` 服务端，等待连接。
3. 打开 **SPP RTT Client**，允许“附近设备”权限，点击“授权 / 刷新已配对设备”。
4. 选择树莓派，点击“连接”；App 自动回环，RTT 结果在树莓派终端显示。
5. 测试期间保持页面在前台。一次测试结束后，重新运行服务端，再次连接。

### 本地构建（可选）

下面的命令行构建路径支持当前 Debian ARM64 树莓派，不依赖 Android Studio。
脚本下载固定版本的 Kotlin JVM 编译器、Android API 35 平台和 Build Tools 35.0.0，仅使用其中可跨架构运行的 D8 JAR；资源打包使用 Debian 的原生 ARM64 `aapt2` 和 API 33 平台资源表，以兼容该打包器支持的资源格式。Kotlin 编译及目标版本仍为 API 35。

```bash
sudo apt install openjdk-21-jdk-headless aapt apksigner curl unzip zip perl
cd kotlin/bluetooth/classic/sppc_rtt
chmod +x build_apk.sh
./build_apk.sh
apksigner verify --verbose --print-certs build/artifacts/sppc-debug.apk
sha256sum build/artifacts/sppc-debug.apk
```

首次下载约数百 MB，缓存与工具全部放在 `build/tooling/`，不进 Git。
Debug 密钥生成在 `build/debug.keystore`，仅供测试；清理该密钥后重新构建会更换签名，手机可能需要卸载旧版后重新安装。

### GitHub Actions 构建（可选）

工作流为仓库根 `.github/workflows/android-sppc.yml`，使用 Ubuntu x86_64、JDK 17、Gradle 8.11.1、AGP 8.9.1 和 Kotlin 2.0.21。
推送本客户端变更或手动触发后执行 `:app:assembleDebug`，在运行页面下载 `sppc-debug-apk` artifact，解压得到 APK 和 SHA-256 文件。artifact 保存 30 天。
此构建路径需要工作流已提交到 GitHub，并具有可用的 Actions 权限和额度；本地 APK 构建不依赖远端。

## 关键参数 / 配置

| 参数 / 配置 | 默认值 | 说明 |
| :--- | :--- | :--- |
| 应用包名 | `com.raspberry.sppc` | 安装标识 |
| SPP UUID | `00001101-0000-1000-8000-00805f9b34fb` | 与服务端一致 |
| 最低 Android | API 26 | Android 8.0 |
| 目标 Android | API 35 | Android 15 |
| 版本 | `1.1.0` / `2` | versionName / versionCode |
| 帧头长度 | 40 字节 | 与 C++ 服务端协议一致 |
| 最大帧长度 | 65536 字节 | 包含头部 |
| UI 更新间隔 | 500 ms | 回环不等待 UI 更新 |
| 签名 | Debug | 测试安装，不用于商店发布 |

## 已知问题 / 注意事项

- APK 构建和签名检查不能替代手机实测。需要手机参与才能验证安装、权限、配对、连接及真实 RTT。
- Android 12+ 需要附近设备中的蓝牙连接和扫描权限。停止发现操作需要扫描权限，App 本身只显示已配对设备。
- App 不提供后台常驻服务；切到后台、锁屏或系统回收会影响测试。测试前台保持亮屏。
- 手机横竖屏切换会重建 Activity 并断开连接，需重新运行服务端和连接。
- 协议 v2 在偏移 24/32 填写 `elapsedRealtimeNanos()` 收齐帧/回传前时间戳，所有统计由树莓派完成；需要两端同时更新，旧版协议不兼容。
- 树莓派输出往/返估计值及手机处理实测值，单向估计依赖双向对称校准和等速时钟假设；时钟漂移与链路不对称会产生偏差。
- RFCOMM 是字节流，App 按协议长度读完整帧后一次写回；无效头或超长帧会关闭连接。
- 构建缓存较大，手写业务源码较小；`build/`、`.gradle/`、APK 和密钥已忽略。

## 参考资料

- [Android 经典蓝牙连接](https://developer.android.com/develop/connectivity/bluetooth/connect-bluetooth-devices)
- [Android 蓝牙权限](https://developer.android.com/develop/connectivity/bluetooth/bt-permissions)
- [Android 蓝牙数据传输](https://developer.android.com/develop/connectivity/bluetooth/transfer-data)
