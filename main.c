#include <stdio.h>

#include <gio/gio.h>

#include "headers/songInfo.h"
#include "headers/DBusFunctions.h"
#include "headers/network.h"

int main(void)
{
    GDBusConnection* DBusConnection = getPlayerDBusConnection();
    if (DBusConnection == NULL) {
        printf("DBusError\n");
        return 1;
    }

    printf("%d\n", getTrackPosition(DBusConnection, "kew"));
    SongData metadata = getTrackMetadata(DBusConnection, "kew");
    printf("%s\n", metadata.album);

    g_object_unref(DBusConnection);
    deallocateSongData(metadata);

    return 0;
}
