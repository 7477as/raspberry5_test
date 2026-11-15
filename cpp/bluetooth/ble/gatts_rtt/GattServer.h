#pragma once
#include <gio/gio.h>
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

/** BlueZ local GATT service: notifications out, write commands in. */
class GattServer {
public:
    explicit GattServer(const std::string& adapter);
    ~GattServer();
    GattServer(const GattServer&) = delete;
    GattServer& operator=(const GattServer&) = delete;
    void Pump();
    bool Ready() const { return mReady; }
    void Send(const std::array<std::uint8_t, 20>& frame);
    std::function<void(const std::uint8_t*, std::size_t)> Receive;
private:
    static void Method(GDBusConnection*, const gchar*, const gchar*, const gchar*, const gchar*, GVariant*, GDBusMethodInvocation*, gpointer);
    static GVariant* Property(GDBusConnection*, const gchar*, const gchar*, const gchar*, const gchar*, GError**, gpointer);
    void Register(const char* interface, const char* method, const char* path);
    GVariant* Properties(const std::string& path, const std::string& interface);
    void Cleanup();
    GDBusConnection* mBus = nullptr;
    GDBusNodeInfo* mInfo = nullptr;
    std::vector<unsigned> mObjects;
    std::string mAdapter;
    bool mReady = false;
    bool mApplication = false;
    bool mAdvertisement = false;
    std::string mPeer;
    std::array<std::uint8_t, 20> mValue{};
};
