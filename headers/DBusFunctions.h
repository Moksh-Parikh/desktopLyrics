#pragma once

#include <stdint.h>
#include <gio/gio.h>

#include "songInfo.h"

#define MPRIS_DESTINATION_ROOT "org.mpris.MediaPlayer2."

GDBusConnection* getPlayerDBusConnection();
GVariant* sendDBusQuery(GDBusConnection* connection, char* player, char* propertyName);
uint64_t getTrackPosition(GDBusConnection* connection, char* player);
SongData getTrackMetadata(GDBusConnection* connection, char* player);
