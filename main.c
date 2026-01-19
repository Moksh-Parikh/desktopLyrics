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

    char* playerName = getCurrentPlayer(DBusConnection);
    
    printf("%d\n", getTrackPosition(DBusConnection, playerName));
    SongData metadata = getTrackMetadata(DBusConnection, playerName);
    printf("%s\n", metadata.album);
    
    Lyrics* lyrics = getSyncedLyricsFromLIBLRC(&metadata);
    
    if (lyrics != NULL)
        printf("%s\n", lyrics->lines[0]);

    g_object_unref(DBusConnection);
    deallocateSongData(metadata);
    free(playerName);

    return 0;
}
