#include <stdio.h>
#include <stdint.h>

#include <gio/gio.h>

#include "headers/DBusFunctions.h"

GDBusConnection* getPlayerDBusConnection() {
    GDBusConnection *connection;
    GError *error = NULL;

    connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if (!connection) {
        g_printerr("Failed to connect to session bus: %s\n", error->message);
        g_error_free(error);
        return NULL;
    }

    return connection;
}

GVariant* sendDBusQuery(GDBusConnection* connection, char* player, char* propertyName) {
    GVariant *result;
    GVariant *value;
    GError *error = NULL;

    char destination[256];
    snprintf(destination, 255, "%s%s", MPRIS_DESTINATION_ROOT, player);

    result = g_dbus_connection_call_sync(
        connection,
        destination,
        "/org/mpris/MediaPlayer2",
        "org.freedesktop.DBus.Properties",
        "Get",
        g_variant_new("(ss)",
                      "org.mpris.MediaPlayer2.Player",
                      propertyName),
        G_VARIANT_TYPE("(v)"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (!result) {
        g_printerr("D-Bus call failed: %s\n", error->message);
        g_error_free(error);
        g_object_unref(connection);
        return NULL;
    }

    g_variant_get(result, "(v)", &value);
    g_variant_unref(result);

    return value;
}

uint64_t getTrackPosition(GDBusConnection* connection, char* player) {
    GVariant* positionGVar = sendDBusQuery(connection, player, "Position");
    uint64_t position = g_variant_get_int64(positionGVar);

    g_variant_unref(positionGVar);
    return position;
}
