#include <stdio.h>
#include <stdint.h>
#include <limits.h>

#include <gio/gio.h>

#include "headers/DBusFunctions.h"
#include "headers/songInfo.h"

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

SongData getTrackMetadata(GDBusConnection* connection, char* player) {
    SongData output = initialiseSongData();

    GVariant *temp;
    char artistArray[256] = {'\0'};

    GVariant* value = sendDBusQuery(connection, player, "Metadata");

    temp = g_variant_lookup_value(value, "xesam:title", NULL);
    if (temp) {
        const char* tempString = g_variant_get_string(temp, NULL);
        output.title = calloc(strlen(tempString) + 1, sizeof(char));
        snprintf(output.title, strlen(tempString) + 1, "%s", tempString);
        g_variant_unref(temp);
    }

    temp = g_variant_lookup_value(value, "xesam:album", NULL);
    if (temp) {
        const char* tempString = g_variant_get_string(temp, NULL);
        output.album = calloc(strlen(tempString) + 1, sizeof(char));
        snprintf(output.album, strlen(tempString) + 1, "%s", tempString);
        g_variant_unref(temp);
    }

    temp = g_variant_lookup_value(value, "xesam:artist", NULL);
    if (temp) {
        GVariantIter iter;
        const gchar *artist;

        g_variant_iter_init(&iter, temp);
        while (g_variant_iter_next(&iter, "&s", &artist)) {
            if (artistArray[0] == '\0') strncat(artistArray, artist, 255);
            else {
                char tempArray[256];
                snprintf(tempArray, 255, " %s", artist);
                strncat(artistArray, artist, 255);
            }
        }
        g_variant_unref(temp);
    }

    temp = g_variant_lookup_value(value, "mpris:length", NULL);
    if (temp) {
        uint64_t length = g_variant_get_int64(temp);
        output.duration = length / 1000000.0f; // microseconds to seconds
        g_variant_unref(temp);
    }

    // temp = g_variant_lookup_value(value, "mpris:artURL", NULL);
    // if (temp) {
    //     snprintf(output.coverArtPath, PATH_MAX - 1, "%s", g_variant_get_string(temp, NULL));
    //     g_variant_unref(temp);
    // }

    g_variant_unref(value);

    return output;
}
