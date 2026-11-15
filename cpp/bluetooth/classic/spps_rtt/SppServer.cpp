#include "SppServer.h"

#include <gio/gunixfdlist.h>
#include <unistd.h>
#include <stdexcept>

namespace {
constexpr const char* kPath = "/com/raspberry/spps/profile";
constexpr const char* kUuid = "00001101-0000-1000-8000-00805f9b34fb";
constexpr const char* kXml = R"(<node><interface name="org.bluez.Profile1">
<method name="Release"/>
<method name="NewConnection"><arg type="o" direction="in"/><arg type="h" direction="in"/><arg type="a{sv}" direction="in"/></method>
<method name="RequestDisconnection"><arg type="o" direction="in"/></method>
</interface></node>)";

void CheckError(GError* error) {
    if (!error) return;
    std::string message = error->message;
    g_error_free(error);
    throw std::runtime_error(message);
}
}

SppServer::SppServer() {
    try {
        GError* error = nullptr;
        mBus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error);
        CheckError(error);
        mInfo = g_dbus_node_info_new_for_xml(kXml, &error);
        CheckError(error);
        static const GDBusInterfaceVTable vtable = {OnMethod, nullptr, nullptr, {nullptr}};
        mObjectId = g_dbus_connection_register_object(mBus, kPath, mInfo->interfaces[0],
                                                     &vtable, this, nullptr, &error);
        CheckError(error);
        GVariantBuilder options;
        g_variant_builder_init(&options, G_VARIANT_TYPE("a{sv}"));
        g_variant_builder_add(&options, "{sv}", "Name", g_variant_new_string("RPi5_SPP_RTT"));
        g_variant_builder_add(&options, "{sv}", "Role", g_variant_new_string("server"));
        g_variant_builder_add(&options, "{sv}", "RequireAuthentication", g_variant_new_boolean(TRUE));
        g_variant_builder_add(&options, "{sv}", "RequireAuthorization", g_variant_new_boolean(FALSE));
        GVariant* reply = g_dbus_connection_call_sync(mBus, "org.bluez", "/org/bluez",
            "org.bluez.ProfileManager1", "RegisterProfile",
            g_variant_new("(osa{sv})", kPath, kUuid, &options), nullptr,
            G_DBUS_CALL_FLAGS_NONE, 10000, nullptr, &error);
        CheckError(error);
        if (reply) g_variant_unref(reply);
        mRegistered = true;
    } catch (...) {
        if (mObjectId) g_dbus_connection_unregister_object(mBus, mObjectId);
        if (mInfo) g_dbus_node_info_unref(mInfo);
        if (mBus) g_object_unref(mBus);
        throw;
    }
}

SppServer::~SppServer() {
    if (mSocket >= 0) close(mSocket);
    if (mRegistered && !mReleased) {
        GVariant* reply = g_dbus_connection_call_sync(mBus, "org.bluez", "/org/bluez",
            "org.bluez.ProfileManager1", "UnregisterProfile", g_variant_new("(o)", kPath),
            nullptr, G_DBUS_CALL_FLAGS_NONE, 1000, nullptr, nullptr);
        if (reply) g_variant_unref(reply);
    }
    if (mObjectId) g_dbus_connection_unregister_object(mBus, mObjectId);
    if (mInfo) g_dbus_node_info_unref(mInfo);
    if (mBus) g_object_unref(mBus);
}

void SppServer::OnMethod(GDBusConnection*, const gchar*, const gchar*, const gchar*,
                         const gchar* method, GVariant* parameters,
                         GDBusMethodInvocation* invocation, gpointer userData) {
    auto& server = *static_cast<SppServer*>(userData);
    if (std::string(method) == "NewConnection") {
        if (server.mSocket >= 0) {
            g_dbus_method_invocation_return_dbus_error(invocation, "org.bluez.Error.Rejected", "Already connected");
            return;
        }
        const gchar* device = nullptr;
        gint32 index = -1;
        GVariant* properties = nullptr;
        g_variant_get(parameters, "(&oh@a{sv})", &device, &index, &properties);
        g_variant_unref(properties);
        GUnixFDList* descriptors = g_dbus_message_get_unix_fd_list(g_dbus_method_invocation_get_message(invocation));
        GError* error = nullptr;
        server.mSocket = descriptors ? g_unix_fd_list_get(descriptors, index, &error) : -1;
        if (server.mSocket < 0) {
            g_dbus_method_invocation_return_dbus_error(invocation, "org.bluez.Error.Rejected",
                                                      error ? error->message : "Missing descriptor");
            if (error) g_error_free(error);
            return;
        }
    } else if (std::string(method) == "Release") {
        server.mReleased = true;
    } else if (std::string(method) == "RequestDisconnection") {
        if (server.mSocket >= 0) close(server.mSocket);
        server.mSocket = -1;
    }
    g_dbus_method_invocation_return_value(invocation, nullptr);
}

int SppServer::WaitForConnection() {
    while (mSocket < 0 && !mReleased) g_main_context_iteration(nullptr, TRUE);
    if (mReleased) throw std::runtime_error("BlueZ released SPP profile");
    return mSocket;
}
