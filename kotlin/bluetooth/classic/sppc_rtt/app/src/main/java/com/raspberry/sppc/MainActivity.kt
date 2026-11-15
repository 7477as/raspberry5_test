package com.raspberry.sppc

import android.Manifest
import android.app.Activity
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothManager
import android.bluetooth.BluetoothSocket
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import android.provider.Settings
import android.view.WindowManager
import android.widget.ArrayAdapter
import android.widget.Button
import android.widget.LinearLayout
import android.widget.Spinner
import android.widget.TextView
import java.io.IOException
import java.util.UUID

class MainActivity : Activity() {
    private lateinit var status: TextView
    private lateinit var devices: Spinner
    private lateinit var connect: Button
    private val handler = Handler(Looper.getMainLooper())
    private var paired = emptyList<BluetoothDevice>()
    private val guard = Any()
    private var generation = 0
    private var socket: BluetoothSocket? = null
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        val layout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(32, 48, 32, 32)
        }
        layout.addView(TextView(this).apply {
            text = "SPP RTT 回环客户端\n先在系统设置中配对树莓派，再刷新列表。连接后会立即原样回传测试帧。\n测试时保持本页面在前台。"
            textSize = 18f
        })
        devices = Spinner(this)
        layout.addView(devices)
        layout.addView(Button(this).apply {
            text = "蓝牙设置 / 配对"
            setOnClickListener { startActivity(Intent(Settings.ACTION_BLUETOOTH_SETTINGS)) }
        })
        layout.addView(Button(this).apply {
            text = "授权 / 刷新已配对设备"
            setOnClickListener { refresh() }
        })
        connect = Button(this).apply {
            text = "连接"
            setOnClickListener { startConnection() }
        }
        layout.addView(connect)
        layout.addView(Button(this).apply {
            text = "断开"
            setOnClickListener { disconnect(); status.text = "已断开" }
        })
        status = TextView(this).apply { text = "尚未连接"; textSize = 16f }
        layout.addView(status)
        setContentView(layout)
        refresh()
    }

    private fun hasPermissions(): Boolean = Build.VERSION.SDK_INT < 31 ||
        (checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED &&
         checkSelfPermission(Manifest.permission.BLUETOOTH_SCAN) == PackageManager.PERMISSION_GRANTED)

    private fun refresh() {
        if (!hasPermissions()) {
            requestPermissions(arrayOf(Manifest.permission.BLUETOOTH_CONNECT, Manifest.permission.BLUETOOTH_SCAN), 1)
            return
        }
        try {
            val adapter = getSystemService(BluetoothManager::class.java)?.adapter
            if (adapter == null || !adapter.isEnabled) {
                status.text = "请在系统设置中开启蓝牙。"
                return
            }
            paired = adapter.bondedDevices.sortedBy { it.name ?: it.address }
            devices.adapter = ArrayAdapter(this, android.R.layout.simple_spinner_dropdown_item,
                paired.map { "${it.name ?: "Unknown"} (${it.address})" })
            status.text = "找到 ${paired.size} 个已配对设备。选择运行 spps 的树莓派。"
        } catch (error: SecurityException) {
            status.text = "蓝牙权限不可用：${error.message}"
        }
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (hasPermissions()) refresh() else status.text = "需要允许附近设备权限才能连接蓝牙。"
    }

    private fun report(token: Int, text: String, finished: Boolean = false) {
        handler.post {
            synchronized(guard) {
                if (token == generation) {
                    status.text = text
                    if (finished) connect.isEnabled = true
                }
            }
        }
    }

    private fun startConnection() {
        if (!hasPermissions()) { refresh(); return }
        val device = paired.getOrNull(devices.selectedItemPosition) ?: run {
            status.text = "请先配对并选择树莓派。"
            return
        }
        disconnect()
        val token = synchronized(guard) { generation }
        connect.isEnabled = false
        status.text = "正在连接……"
        Thread({
            var connection: BluetoothSocket? = null
            try {
                getSystemService(BluetoothManager::class.java).adapter.cancelDiscovery()
                connection = device.createRfcommSocketToServiceRecord(UUID.fromString("00001101-0000-1000-8000-00805f9b34fb"))
                synchronized(guard) {
                    if (token != generation) { connection.close(); return@Thread }
                    socket = connection
                }
                connection.connect()
                report(token, "已连接，等待测试帧……")
                echo(connection, token)
                report(token, "服务端已断开，测试结束。", true)
            } catch (error: Exception) {
                report(token, "连接结束：${error.message ?: error.javaClass.simpleName}", true)
            } finally {
                try { connection?.close() } catch (_: IOException) { }
                synchronized(guard) { if (token == generation) socket = null }
            }
        }, "spp-immediate-echo").start()
    }

    private fun echo(connection: BluetoothSocket, token: Int) {
        val input = connection.inputStream
        val output = connection.outputStream
        val frame = ByteArray(65536)
        var frames = 0L
        var bytes = 0L
        var lastUpdate = 0L
        while (true) {
            if (!readFully(input, frame, 0, 40)) return
            if (readNumber(frame, 0, 4) != 0x53505031L || frame[4].toInt() != 2 ||
                frame[5].toInt() != 1 || readNumber(frame, 6, 2) != 40L)
                throw IOException("无效协议头")
            val length = readNumber(frame, 8, 4)
            if (length < 40 || length > frame.size) throw IOException("无效帧长度")
            if (!readFully(input, frame, 40, length.toInt() - 40)) throw IOException("帧未完整接收")
            val receivedAt = SystemClock.elapsedRealtimeNanos()
            writeNumber(frame, 24, receivedAt)
            writeNumber(frame, 32, SystemClock.elapsedRealtimeNanos())
            output.write(frame, 0, length.toInt())
            frames++
            bytes += length
            val now = SystemClock.elapsedRealtime()
            if (now - lastUpdate >= 500) {
                report(token, "已连接\n已回传 $frames 帧 / $bytes 字节")
                lastUpdate = now
            }
        }
    }

    private fun writeNumber(bytes: ByteArray, offset: Int, value: Long) {
        for (index in 0 until 8) bytes[offset + index] = (value ushr ((7 - index) * 8)).toByte()
    }

    private fun readNumber(bytes: ByteArray, offset: Int, length: Int): Long {
        var value = 0L
        for (index in offset until offset + length) value = (value shl 8) or (bytes[index].toLong() and 255)
        return value
    }

    private fun readFully(input: java.io.InputStream, frame: ByteArray, start: Int, length: Int): Boolean {
        var offset = start
        while (offset < start + length) {
            val count = input.read(frame, offset, start + length - offset)
            if (count < 0) {
                if (offset == 0) return false
                throw IOException("接收中断：半帧断连")
            }
            offset += count
        }
        return true
    }

    private fun disconnect() {
        val previous = synchronized(guard) {
            generation++
            val previous = socket
            socket = null
            previous
        }
        try { previous?.close() } catch (error: IOException) { status.text = "关闭连接：${error.message}" }
        connect.isEnabled = true
    }

    override fun onDestroy() {
        disconnect()
        handler.removeCallbacksAndMessages(null)
        super.onDestroy()
    }
}
