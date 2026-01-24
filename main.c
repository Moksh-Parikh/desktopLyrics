#include <stdio.h>
#include <math.h>
#include <gio/gio.h>

#include "headers/songInfo.h"
#include "headers/DBusFunctions.h"
#include "headers/network.h"

#define ONE_BILLION 1000000000L

struct timespec nanosleepWrapper(double seconds) {
    struct timespec request, remaining;

    if (seconds > 1.0f) {
        request.tv_sec = seconds - (int)seconds;
    }

    request.tv_nsec = (uint64_t)( ( seconds - (int)seconds ) * (double)ONE_BILLION );
    
    nanosleep(
        &request,
        &remaining
    );

    return remaining;
}

int findClosest(double* array, int n, double target) {
    int low = 0, mid, high = n - 1;

    while (low <= high) {
        mid = low + (high - low) / 2;

        if (array[mid] == target) {
            return array[mid];
        }
        else if (array[mid] < target) {
            low = mid + 1;
        }
        else {
            high = mid - 1;
        }
    }
    return mid;
}

void printLyrics(GDBusConnection* DBusConnection,
                const gchar* senderName,
                const gchar* objectPath,
                const gchar* interfaceName,
                const gchar* signalName,
                GVariant* parameters,
                gpointer user_data
) {
    double trackPosition = 0;
    char* playerName = getCurrentPlayer(DBusConnection);
    SongData metadata = getTrackMetadata(DBusConnection, playerName);

    getSyncedLyricsFromLIBLRC(&metadata);
    if (metadata.lyrics == NULL || metadata.lyrics->lines == NULL) {
        printf("Lyric error\n");
        return;
    }

    double* timeArray = generateLyricTimeArray(metadata.lyrics);
    int currentLyricLine = 0;
    
    trackPosition = getTrackPosition(DBusConnection, playerName);
    currentLyricLine = findClosest(timeArray, metadata.lyrics->count, trackPosition);
    
    while (currentLyricLine < metadata.lyrics->count) {
        trackPosition = getTrackPosition(DBusConnection, playerName);

        if (trackPosition > (timeArray[currentLyricLine] - 0.5f) &&
            trackPosition < (timeArray[currentLyricLine] + 0.5f)
        ) {
            printf("%s\n", metadata.lyrics->lines[currentLyricLine].text);
            currentLyricLine++;
        }
        else if (trackPosition > (timeArray[currentLyricLine] + 0.5f) ) {
            currentLyricLine = findClosest(timeArray, metadata.lyrics->count, trackPosition);
        }

        nanosleepWrapper(timeArray[currentLyricLine] -
                         timeArray[currentLyricLine - 1] - 0.25f
                        );
    }
    deallocateSongData(metadata);
    free(playerName);
    free(timeArray);
}

int main(void) {
    GMainLoop *loop;
    GDBusConnection* DBusConnection = getPlayerDBusConnection();
    if (DBusConnection == NULL) {
        printf("DBusError\n");
        return 1;
    }

    printLyrics(DBusConnection, NULL,NULL,NULL,NULL,NULL,NULL);

    g_dbus_connection_signal_subscribe(
        DBusConnection,
        NULL,   /* any sender */
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        "/org/mpris/MediaPlayer2",
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        printLyrics,
        NULL,
        NULL
    );
    // printLyrics(DBusConnection, NULL, NULL, NULL, NULL, NULL, NULL);
    loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    g_main_loop_unref(loop);
    g_object_unref(DBusConnection);

    return 0;
}
