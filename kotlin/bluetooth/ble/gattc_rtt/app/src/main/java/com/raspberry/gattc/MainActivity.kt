package com.raspberry.gattc

import android.Manifest
import android.app.Activity
import android.bluetooth.*
import android.bluetooth.le.*
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.HandlerThread
import android.os.Looper
import android.os.ParcelUuid
import android.os.SystemClock
import android.view.WindowManager
import android.widget.*
import java.util.UUID
import java.security.SecureRandom

@Suppress("DEPRECATION")
class MainActivity : Activity() {
    private val serviceId = UUID.fromString("d3b5f100-6172-4c29-a563-45b9c8e57001")
    private val writeId = UUID.fromString("d3b5f101-6172-4c29-a563-45b9c8e57001")
    private val notifyId = UUID.fromString("d3b5f102-6172-4c29-a563-45b9c8e57001")
    private val cccId = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")
    private val ui = Handler(Looper.getMainLooper())
    private val thread = HandlerThread("gatt-rtt").apply { start() }
    private val worker = Handler(thread.looper)
    private lateinit var status: TextView
    private lateinit var devices: LinearLayout
    private val found = mutableSetOf<String>()
    private var adapter: BluetoothAdapter? = null
    private var scanning = false
    private var gatt: BluetoothGatt? = null
    private var writer: BluetoothGattCharacteristic? = null
    private var busy = false
    private var ready = false
    private var frames = 0L
    private var session = 0L
    private var lastUi = 0L
    private var generation = 0
    private var mtu = 23
    private var phy = "unknown"
    private var phase = "idle"
    private var scanCallback: ScanCallback? = null
    private val operationTimeout = Runnable { closeConnection("GATT 操作超时：$phase") }

    override fun onCreate(state: Bundle?) {
        super.onCreate(state)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        val layout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(24, 24, 24, 24)
        }
        status = TextView(this).apply { text = "GATT RTT Client 1.0\n先启动树莓派 gatts，再扫描连接。" }
        layout.addView(status)
        layout.addView(Button(this).apply {
            text = "扫描 BLE RTT 服务"
            setOnClickListener { requestScan() }
        })
        layout.addView(Button(this).apply {
            text = "断开"
            setOnClickListener { stopScan(); worker.post { closeConnection("已断开") } }
        })
        devices = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL }
        layout.addView(ScrollView(this).apply { addView(devices) })
        setContentView(layout)
        adapter = getSystemService(BluetoothManager::class.java)?.adapter
    }

    private fun show(text: String) { ui.post { status.text = text } }
    private fun requestScan() {
        val permissions = if (Build.VERSION.SDK_INT >= 31)
            arrayOf(Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.BLUETOOTH_CONNECT)
        else arrayOf(Manifest.permission.ACCESS_FINE_LOCATION)
        val missing = permissions.filter { checkSelfPermission(it) != PackageManager.PERMISSION_GRANTED }
        if (missing.isNotEmpty()) requestPermissions(missing.toTypedArray(), 1) else startScan()
    }
    override fun onRequestPermissionsResult(code: Int, permissions: Array<out String>, results: IntArray) {
        super.onRequestPermissionsResult(code, permissions, results)
        if (code == 1 && results.isNotEmpty() && results.all { it == PackageManager.PERMISSION_GRANTED }) startScan()
        else show("扫描需要蓝牙权限；Android 8～11 还需位置权限和系统定位开关。")
    }
    private fun startScan() {
        stopScan()
        if (adapter?.isEnabled != true) { show("请先打开系统蓝牙"); return }
        found.clear(); devices.removeAllViews()
        val callback = object : ScanCallback() {
            override fun onScanResult(type: Int, result: ScanResult) {
                ui.post {
                    if (!scanning || !found.add(result.device.address)) return@post
                    devices.addView(Button(this@MainActivity).apply {
                        text = "${result.scanRecord?.deviceName ?: "GATT RTT"}\n${result.device.address} RSSI=${result.rssi}"
                        setOnClickListener { stopScan(); worker.post { connect(result.device) } }
                    })
                }
            }
            override fun onScanFailed(code: Int) { scanning = false; show("扫描失败：$code") }
        }
        scanCallback = callback
        try {
            adapter?.bluetoothLeScanner?.startScan(
                listOf(ScanFilter.Builder().setServiceUuid(ParcelUuid(serviceId)).build()),
                ScanSettings.Builder().setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build(), callback)
            scanning = true
            show("扫描 15 秒；选择 RPi5_GATT_RTT。BLE 无须预先配对。")
            ui.postDelayed({ if (scanCallback === callback) stopScan() }, 15000)
        } catch (error: Exception) { show("扫描失败：${error.message}") }
    }
    private fun stopScan() {
        if (scanning) try { scanCallback?.let { adapter?.bluetoothLeScanner?.stopScan(it) } }
        catch (error: Exception) { show("停止扫描失败：${error.message}") }
        scanning = false
        scanCallback = null
    }
    private fun armTimeout(next: String) {
        phase = next
        worker.removeCallbacks(operationTimeout)
        worker.postDelayed(operationTimeout, 15000)
    }
    private fun connect(device: BluetoothDevice) {
        closeConnection("连接中……")
        val token = generation
        val callback = object : BluetoothGattCallback() {
            override fun onConnectionStateChange(connection: BluetoothGatt, code: Int, state: Int) {
                if (token != generation) { connection.close(); return }
                if (code != BluetoothGatt.GATT_SUCCESS || state == BluetoothProfile.STATE_DISCONNECTED) {
                    closeConnection("连接结束：GATT status=$code state=$state")
                    return
                }
                if (state == BluetoothProfile.STATE_CONNECTED) {
                    connection.requestConnectionPriority(BluetoothGatt.CONNECTION_PRIORITY_HIGH)
                    armTimeout("服务发现")
                    if (!connection.discoverServices()) closeConnection("无法发起服务发现")
                }
            }
            override fun onServicesDiscovered(connection: BluetoothGatt, code: Int) {
                if (token != generation) return
                val service = connection.getService(serviceId)
                val rx = service?.getCharacteristic(writeId)
                val tx = service?.getCharacteristic(notifyId)
                if (code != BluetoothGatt.GATT_SUCCESS || rx == null || tx == null ||
                    rx.properties and BluetoothGattCharacteristic.PROPERTY_WRITE_NO_RESPONSE == 0 ||
                    tx.properties and BluetoothGattCharacteristic.PROPERTY_NOTIFY == 0) {
                    closeConnection("服务/特征缺失或权限错误：$code"); return
                }
                writer = rx
                armTimeout("MTU 协商")
                if (!connection.requestMtu(23)) subscribe(connection)
            }
            override fun onMtuChanged(connection: BluetoothGatt, value: Int, code: Int) {
                if (token != generation) return
                if (code == BluetoothGatt.GATT_SUCCESS) mtu = value
                if (phase == "MTU 协商") subscribe(connection)
            }
            override fun onDescriptorWrite(connection: BluetoothGatt, descriptor: BluetoothGattDescriptor, code: Int) {
                if (token != generation) return
                if (code != BluetoothGatt.GATT_SUCCESS) { closeConnection("Notify 订阅失败：$code"); return }
                if (Build.VERSION.SDK_INT >= 26) {
                    connection.setPreferredPhy(BluetoothDevice.PHY_LE_2M_MASK, BluetoothDevice.PHY_LE_2M_MASK, BluetoothDevice.PHY_OPTION_NO_PREFERRED)
                    connection.readPhy()
                }
                armTimeout("连接参数稳定")
                worker.postDelayed({
                    if (token == generation) {
                        session = SecureRandom().nextInt().toLong() and 0xfff00000L
                        val hello = ByteArray(20)
                        write(hello, 0, 0xffffffffL, 4)
                        write(hello, 4, session, 8)
                        write(hello, 12, 0x4741545452545431L, 8)
                        ready = true
                        send(connection, hello)
                        show("已订阅，等待树莓派测试帧\nMTU=$mtu PHY=$phy；2M/高优先级为请求值。")
                    }
                }, 1000)
            }
            override fun onPhyUpdate(connection: BluetoothGatt, tx: Int, rx: Int, code: Int) {
                if (token == generation && code == BluetoothGatt.GATT_SUCCESS) phy = "TX=$tx RX=$rx (1=1M,2=2M,3=Coded)"
            }
            override fun onPhyRead(connection: BluetoothGatt, tx: Int, rx: Int, code: Int) {
                onPhyUpdate(connection, tx, rx, code)
            }
            override fun onCharacteristicChanged(connection: BluetoothGatt, characteristic: BluetoothGattCharacteristic, value: ByteArray) {
                if (token == generation && characteristic.uuid == notifyId) echo(connection, value)
            }
            override fun onCharacteristicChanged(connection: BluetoothGatt, characteristic: BluetoothGattCharacteristic) {
                if (Build.VERSION.SDK_INT < 33 && token == generation && characteristic.uuid == notifyId)
                    echo(connection, characteristic.value.copyOf())
            }
            override fun onCharacteristicWrite(connection: BluetoothGatt, characteristic: BluetoothGattCharacteristic, code: Int) {
                if (token != generation) return
                busy = false
                worker.removeCallbacks(operationTimeout)
                phase = "测试中"
                if (code != BluetoothGatt.GATT_SUCCESS) closeConnection("写失败：$code")
            }
        }
        try {
            armTimeout("连接")
            gatt = device.connectGatt(this, false, callback, BluetoothDevice.TRANSPORT_LE, BluetoothDevice.PHY_LE_1M_MASK, worker)
            if (gatt == null) closeConnection("connectGatt 失败")
        } catch (error: Exception) { closeConnection("连接失败：${error.message}") }
    }
    private fun subscribe(connection: BluetoothGatt) {
        val tx = connection.getService(serviceId)?.getCharacteristic(notifyId)
        val ccc = tx?.getDescriptor(cccId)
        if (tx == null || ccc == null || !connection.setCharacteristicNotification(tx, true)) {
            closeConnection("无法开启 Notify/缺少 CCCD"); return
        }
        armTimeout("Notify 订阅")
        val accepted = if (Build.VERSION.SDK_INT >= 33)
            connection.writeDescriptor(ccc, BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE) == BluetoothStatusCodes.SUCCESS
        else {
            ccc.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
            connection.writeDescriptor(ccc)
        }
        if (!accepted) closeConnection("CCCD 写请求未被接受")
    }
    private fun echo(connection: BluetoothGatt, value: ByteArray) {
        val received = SystemClock.elapsedRealtimeNanos()
        if (!ready || value.size != 20 || read(value, 12, 8) != 0x4741545452545431L ||
            (read(value, 0, 4) and 0xfff00000L) != session) {
            closeConnection("无效协议帧/会话不匹配"); return
        }
        val reply = ByteArray(20)
        value.copyInto(reply, 0, 0, 4)
        write(reply, 4, received, 8)
        write(reply, 12, SystemClock.elapsedRealtimeNanos(), 8)
        send(connection, reply)
        frames++
        val now = SystemClock.elapsedRealtime()
        if (now - lastUi > 500) {
            lastUi = now
            show("已回传 $frames 帧\nMTU=$mtu PHY=$phy\nNotify → 无响应写；统计见树莓派。")
        }
    }
    private fun send(connection: BluetoothGatt, value: ByteArray) {
        val characteristic = writer ?: return
        if (busy) { closeConnection("GATT busy：写回包未完成，请降低频率或窗口"); return }
        busy = true
        armTimeout("写完成回调")
        try {
            if (read(value, 0, 4) != 0xffffffffL)
                write(value, 12, SystemClock.elapsedRealtimeNanos(), 8)
            val accepted = if (Build.VERSION.SDK_INT >= 33)
                connection.writeCharacteristic(characteristic, value, BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE) == BluetoothStatusCodes.SUCCESS
            else {
                characteristic.writeType = BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE
                characteristic.value = value
                connection.writeCharacteristic(characteristic)
            }
            if (!accepted) closeConnection("无响应写请求未被接受")
        } catch (error: Exception) { closeConnection("写异常：${error.message}") }
    }
    private fun read(bytes: ByteArray, offset: Int, count: Int): Long {
        var value = 0L
        for (index in offset until offset + count) value = (value shl 8) or (bytes[index].toLong() and 255)
        return value
    }
    private fun write(bytes: ByteArray, offset: Int, value: Long, count: Int) {
        for (index in 0 until count) bytes[offset + index] = (value ushr ((count - 1 - index) * 8)).toByte()
    }
    private fun closeConnection(message: String) {
        generation++
        worker.removeCallbacksAndMessages(null)
        ready = false; busy = false; writer = null; frames = 0; mtu = 23; phy = "unknown"; phase = "idle"
        val previous = gatt
        gatt = null
        try { previous?.disconnect() } finally { previous?.close() }
        show(message)
    }
    override fun onDestroy() {
        stopScan()
        worker.post { closeConnection("已关闭"); thread.quitSafely() }
        super.onDestroy()
    }
}
