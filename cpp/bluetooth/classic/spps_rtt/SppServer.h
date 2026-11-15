#pragma once

#include <gio/gio.h>
#include <string>

/** Registers a BlueZ SPP profile and owns one accepted RFCOMM connection. */
class SppServer {
public:
    SppServer();
    ~SppServer();
    SppServer(const SppServer&) = delete;
    SppServer& operator=(const SppServer&) = delete;
    int WaitForConnection();
private:
    static void OnMethod(GDBusConnection*, const gchar*, const gchar*, const gchar*,
                         const gchar*, GVariant*, GDBusMethodInvocation*, gpointer);
    GDBusConnection* mBus = nullptr;
    GDBusNodeInfo* mInfo = nullptr;
    guint mObjectId = 0;
    int mSocket = -1;
    bool mReleased = false;
    bool mRegistered = false;
};
