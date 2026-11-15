#include "GattServer.h"
#include <iostream>
#include <stdexcept>

namespace {
constexpr const char* kRoot = "/com/raspberry/gatts";
constexpr const char* kService = "/com/raspberry/gatts/service0";
constexpr const char* kRx = "/com/raspberry/gatts/service0/rx";
constexpr const char* kTx = "/com/raspberry/gatts/service0/tx";
constexpr const char* kAdvert = "/com/raspberry/gatts/advert";
constexpr const char* kServiceUuid = "d3b5f100-6172-4c29-a563-45b9c8e57001";
constexpr const char* kRxUuid = "d3b5f101-6172-4c29-a563-45b9c8e57001";
constexpr const char* kTxUuid = "d3b5f102-6172-4c29-a563-45b9c8e57001";
constexpr const char* kXml = R"(<node>
<interface name="org.freedesktop.DBus.ObjectManager"><method name="GetManagedObjects"><arg type="a{oa{sa{sv}}}" direction="out"/></method></interface>
<interface name="org.bluez.GattService1"><property name="UUID" type="s" access="read"/><property name="Primary" type="b" access="read"/></interface>
<interface name="org.bluez.GattCharacteristic1">
<method name="WriteValue"><arg type="ay" direction="in"/><arg type="a{sv}" direction="in"/></method>
<method name="StartNotify"/><method name="StopNotify"/>
<property name="UUID" type="s" access="read"/><property name="Service" type="o" access="read"/>
<property name="Flags" type="as" access="read"/><property name="Notifying" type="b" access="read"/>
<property name="Value" type="ay" access="read"/>
</interface>
<interface name="org.bluez.LEAdvertisement1"><method name="Release"/>
<property name="Type" type="s" access="read"/><property name="ServiceUUIDs" type="as" access="read"/>
<property name="LocalName" type="s" access="read"/></interface>
</node>)";
void Check(GError* error) {
    if (!error) return;
    const std::string text = error->message;
    g_error_free(error);
    throw std::runtime_error(text);
}
}

GattServer::GattServer(const std::string& adapter) : mAdapter("/org/bluez/" + adapter) {
    try {
        GError* error = nullptr;
        mBus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error);
        Check(error);
        mInfo = g_dbus_node_info_new_for_xml(kXml, &error);
        Check(error);
        static const GDBusInterfaceVTable table = {Method, Property, nullptr, {nullptr}};
        const std::pair<const char*, unsigned> objects[] = {{kRoot, 0}, {kService, 1}, {kRx, 2}, {kTx, 2}, {kAdvert, 3}};
        for (const auto& object : objects) {
            mObjects.push_back(g_dbus_connection_register_object(mBus, object.first, mInfo->interfaces[object.second], &table, this, nullptr, &error));
            Check(error);
        }
        Register("org.bluez.GattManager1", "RegisterApplication", kRoot);
        mApplication = true;
        Register("org.bluez.LEAdvertisingManager1", "RegisterAdvertisement", kAdvert);
        mAdvertisement = true;
    } catch (...) { Cleanup(); throw; }
}

void GattServer::Register(const char* interface, const char* method, const char* path) {
    struct Completion { bool Done = false; GError* Error = nullptr; } completion;
    g_dbus_connection_call(mBus, "org.bluez", mAdapter.c_str(), interface, method,
        g_variant_new("(oa{sv})", path, nullptr), nullptr, G_DBUS_CALL_FLAGS_NONE, 15000, nullptr,
        [](GObject* object, GAsyncResult* result, gpointer data) {
            auto& state = *static_cast<Completion*>(data);
            auto* reply = g_dbus_connection_call_finish(G_DBUS_CONNECTION(object), result, &state.Error);
            if (reply) g_variant_unref(reply);
            state.Done = true;
        }, &completion);
    while (!completion.Done) g_main_context_iteration(nullptr, TRUE);
    Check(completion.Error);
}

GattServer::~GattServer() { Cleanup(); }
void GattServer::Cleanup() {
    if (mBus) {
        const auto unregister = [&](const char* interface, const char* method, const char* path) {
            auto* reply = g_dbus_connection_call_sync(mBus, "org.bluez", mAdapter.c_str(), interface, method,
                g_variant_new("(o)", path), nullptr, G_DBUS_CALL_FLAGS_NONE, 1000, nullptr, nullptr);
            if (reply) g_variant_unref(reply);
        };
        if (mAdvertisement) unregister("org.bluez.LEAdvertisingManager1", "UnregisterAdvertisement", kAdvert);
        if (mApplication) unregister("org.bluez.GattManager1", "UnregisterApplication", kRoot);
        for (auto object : mObjects) if (object) g_dbus_connection_unregister_object(mBus, object);
        g_object_unref(mBus);
    }
    if (mInfo) g_dbus_node_info_unref(mInfo);
    mBus = nullptr;
    mInfo = nullptr;
}

GVariant* GattServer::Properties(const std::string& path, const std::string& interface) {
    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));
    const auto add = [&](const char* name, GVariant* value) { g_variant_builder_add(&builder, "{sv}", name, value); };
    if (interface == "org.bluez.GattService1") {
        add("UUID", g_variant_new_string(kServiceUuid));
        add("Primary", g_variant_new_boolean(TRUE));
    } else if (interface == "org.bluez.GattCharacteristic1") {
        const bool transmit = path == kTx;
        add("UUID", g_variant_new_string(transmit ? kTxUuid : kRxUuid));
        add("Service", g_variant_new_object_path(kService));
        const char* flags[] = {transmit ? "notify" : "write-without-response", nullptr};
        add("Flags", g_variant_new_strv(flags, -1));
        if (transmit) {
            add("Notifying", g_variant_new_boolean(mReady));
            add("Value", g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE, mValue.data(), mValue.size(), 1));
        }
    } else if (interface == "org.bluez.LEAdvertisement1") {
        add("Type", g_variant_new_string("peripheral"));
        const char* uuids[] = {kServiceUuid, nullptr};
        add("ServiceUUIDs", g_variant_new_strv(uuids, -1));
        add("LocalName", g_variant_new_string("RPi5_GATT_RTT"));
    }
    return g_variant_builder_end(&builder);
}

GVariant* GattServer::Property(GDBusConnection*, const gchar*, const gchar* path, const gchar* interface,
                              const gchar* name, GError**, gpointer data) {
    auto& server = *static_cast<GattServer*>(data);
    auto* properties = g_variant_ref_sink(server.Properties(path, interface));
    auto* value = g_variant_lookup_value(properties, name, nullptr);
    g_variant_unref(properties);
    return value;
}

void GattServer::Method(GDBusConnection*, const gchar*, const gchar* path, const gchar*, const gchar* method,
                        GVariant* parameters, GDBusMethodInvocation* invocation, gpointer data) {
    auto& server = *static_cast<GattServer*>(data);
    const std::string action = method;
    if (action == "GetManagedObjects") {
        GVariantBuilder objects;
        g_variant_builder_init(&objects, G_VARIANT_TYPE("a{oa{sa{sv}}}"));
        for (const char* object : {kService, kRx, kTx}) {
            const char* interface = object == kService ? "org.bluez.GattService1" : "org.bluez.GattCharacteristic1";
            GVariantBuilder interfaces;
            g_variant_builder_init(&interfaces, G_VARIANT_TYPE("a{sa{sv}}"));
            g_variant_builder_add(&interfaces, "{s@a{sv}}", interface, server.Properties(object, interface));
            g_variant_builder_add(&objects, "{oa{sa{sv}}}", object, &interfaces);
        }
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(a{oa{sa{sv}}})", &objects));
        return;
    }
    if (action == "WriteValue" && std::string(path) == kRx) {
        GVariant *bytes = nullptr, *options = nullptr;
        g_variant_get(parameters, "(@ay@a{sv})", &bytes, &options);
        const gchar* device = nullptr;
        g_variant_lookup(options, "device", "&o", &device);
        const bool allowed = device && (server.mPeer.empty() || server.mPeer == device);
        guint16 offset = 0;
        g_variant_lookup(options, "offset", "q", &offset);
        gsize size = 0;
        const auto* value = static_cast<const std::uint8_t*>(g_variant_get_fixed_array(bytes, &size, 1));
        if (allowed && offset == 0 && server.Receive) server.Receive(value, size);
        if (allowed && server.mPeer.empty()) {
            server.mPeer = device;
            guint16 mtu = 23;
            g_variant_lookup(options, "mtu", "q", &mtu);
            std::cout << "Peer=" << device << " ATT_MTU=" << mtu << " transport=BlueZ-D-Bus PHY/connection_interval=unknown (use btmon)\n";
        }
        g_variant_unref(bytes);
        g_variant_unref(options);
        if (!allowed || offset != 0) {
            g_dbus_method_invocation_return_dbus_error(invocation, "org.bluez.Error.NotPermitted", "Single peer only, offset must be zero");
            return;
        }
    } else if (action == "StartNotify") server.mReady = true;
    else if (action == "StopNotify" || action == "Release") server.mReady = false;
    else {
        g_dbus_method_invocation_return_dbus_error(invocation, "org.bluez.Error.NotSupported", "Unsupported method");
        return;
    }
    g_dbus_method_invocation_return_value(invocation, nullptr);
}

void GattServer::Pump() {
    for (unsigned count = 0; count < 64 && g_main_context_pending(nullptr); ++count)
        g_main_context_iteration(nullptr, FALSE);
    if (g_dbus_connection_is_closed(mBus)) throw std::runtime_error("System bus disconnected");
}
void GattServer::Send(const std::array<std::uint8_t, 20>& frame) {
    if (!mReady) throw std::runtime_error("Notifications disabled or peer disconnected");
    mValue = frame;
    GVariantBuilder properties;
    g_variant_builder_init(&properties, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&properties, "{sv}", "Value",
        g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE, frame.data(), frame.size(), 1));
    GError* error = nullptr;
    g_dbus_connection_emit_signal(mBus, nullptr, kTx, "org.freedesktop.DBus.Properties", "PropertiesChanged",
        g_variant_new("(sa{sv}as)", "org.bluez.GattCharacteristic1", &properties, nullptr), &error);
    Check(error);
}
