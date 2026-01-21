#include <stdio.h>
#include <math.h>
#include <gio/gio.h>

#include "headers/songInfo.h"
#include "headers/DBusFunctions.h"
#include "headers/network.h"

#define ONE_BILLION 1000000000L

struct timespec generateTimeSpec(double seconds) {
    struct timespec request;

    if (seconds > 1.0f) {
        request.tv_sec = seconds - (int)seconds;
    }

    request.tv_nsec = (uint64_t)( ( seconds - (int)seconds ) * (double)ONE_BILLION );

    return request;
}

int findClosest(double* array, int n, double target) {
    int result = array[0];
    int low = 0, mid, high = n - 1;

    while (low <= high) {
        mid = low + (high - low) / 2;

        if (abs(array[mid] - target) < abs(result - target)) {
            result = array[mid];
        }
        else if (abs(array[mid] - target) == abs(result - target)) {
            // In case of a tie, the larger value is preferred
            result = (result > array[mid]) ? result : array[mid];
        }

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

int main(void)
{
    GDBusConnection* DBusConnection = getPlayerDBusConnection();
    if (DBusConnection == NULL) {
        printf("DBusError\n");
        return 1;
    }

    char* playerName = getCurrentPlayer(DBusConnection);
    if (playerName == NULL) {
        printf("No players found\n");
        g_object_unref(DBusConnection);
        return 0;
    }

    double trackPosition = getTrackPosition(DBusConnection, playerName);
    printf("%lf\n", trackPosition);
    SongData metadata = getTrackMetadata(DBusConnection, playerName);

    getSyncedLyricsFromLIBLRC(&metadata);
    if (metadata.lyrics == NULL || metadata.lyrics->lines == NULL) {
        printf("Lyric error");
        return 1;
    }

    double* timeArray = generateLyricTimeArray(metadata.lyrics);
    int currentLyricLine = 0;
    
    trackPosition = getTrackPosition(DBusConnection, playerName);
    // for (int i = 0; i < metadata.lyrics->count; i++) {
    //     printf("%lf, %lf\n", timeArray[i], trackPosition);
    //     if (trackPosition > (round(timeArray[i]) - 1.0f) &&
    //         trackPosition < (round(timeArray[i]) + 1.0f)
    //     ) {
    //         currentLyricLine = i;
    //         break;
    //     }
    // }


    currentLyricLine = findClosest(timeArray, metadata.lyrics->count, trackPosition);
    printf("%d\n", currentLyricLine);
    while (currentLyricLine < metadata.lyrics->count) {
        trackPosition = getTrackPosition(DBusConnection, playerName);
        if (trackPosition > (timeArray[currentLyricLine] - 0.5f) &&
            trackPosition < (timeArray[currentLyricLine] + 1.0f)
        ) {
            printf("%s\n", metadata.lyrics->lines[currentLyricLine].text);
            currentLyricLine++;
        }

        struct timespec sleepRequest = generateTimeSpec(timeArray[currentLyricLine] - timeArray[currentLyricLine - 1] - 0.25f);
        nanosleep(
            &sleepRequest,
            NULL
        );
    }

    g_object_unref(DBusConnection);
    deallocateSongData(metadata);
    free(playerName);

    return 0;
}
