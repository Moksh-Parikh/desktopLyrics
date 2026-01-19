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

            output.artist = calloc(strlen(artistArray) + 1, sizeof(char));
            snprintf(output.artist, strlen(artistArray) + 1, "%s", artistArray);
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

char* getCurrentPlayer(GDBusConnection* connection) {
    GVariant *reply;
    GVariantIter *iter;
    GError *error = NULL;
    const gchar *name;

    char* output = NULL;

    reply = g_dbus_connection_call_sync(
        connection,
        "org.freedesktop.DBus",
        "/org/freedesktop/DBus",
        "org.freedesktop.DBus",
        "ListNames",
        NULL,
        G_VARIANT_TYPE("(as)"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (!reply) {
        g_printerr("ListNames failed: %s\n", error->message);
        g_error_free(error);
        return NULL;
    }

    g_variant_get(reply, "(as)", &iter);

    while (g_variant_iter_next(iter, "&s", &name)) {

        if (!g_str_has_prefix(name, "org.mpris.MediaPlayer2."))
            continue;

        /* Query PlaybackStatus */
        GVariant *status_reply = g_dbus_connection_call_sync(
            connection,
            name,
            "/org/mpris/MediaPlayer2",
            "org.freedesktop.DBus.Properties",
            "Get",
            g_variant_new("(ss)",
                          "org.mpris.MediaPlayer2.Player",
                          "PlaybackStatus"),
            G_VARIANT_TYPE("(v)"),
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            NULL,
            NULL
        );

        if (!status_reply)
            continue;

        GVariant *value;
        const gchar *status;

        g_variant_get(status_reply, "(v)", &value);
        status = g_variant_get_string(value, NULL);

        if (g_strcmp0(status, "Playing") == 0) {
            output = calloc(strlen(name) - strlen(MPRIS_DESTINATION_ROOT) + 1, sizeof(char));
            strncpy(output, name + strlen(MPRIS_DESTINATION_ROOT), strlen(name) - strlen(MPRIS_DESTINATION_ROOT));
            printf("%s\n", output);
            g_variant_unref(value);
            g_variant_unref(status_reply);
            break;
        }

        g_variant_unref(value);
        g_variant_unref(status_reply);
    }

    g_variant_iter_free(iter);
    g_variant_unref(reply);

    return output;
}
