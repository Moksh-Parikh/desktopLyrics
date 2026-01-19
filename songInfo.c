#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "headers/songInfo.h"
#include "headers/network.h"

// https://codeberg.org/ravachol/kew
// Credit to ravachol and the other kew contributors
int loadTimedLyrics(FILE *file, Lyrics *lyrics) {
        size_t capacity = 64;
        lyrics->lines = (LyricsLine *)malloc(sizeof(LyricsLine) * capacity);
        if (!lyrics->lines)
                return 0;

        char lineBuffer[1024];

        while (fgets(lineBuffer, sizeof(lineBuffer), file))
        {
                if (lineBuffer[0] != '[' || !isdigit((unsigned char)lineBuffer[1]))
                        continue;

                int min = 0, sec = 0, cs = 0;
                char text[512] = {0};

                if (sscanf(lineBuffer, "[%d:%d.%d]%511[^\r\n]", &min, &sec, &cs, text) == 4)
                {
                        if (lyrics->count == capacity)
                        {
                                capacity *= 2;
                                LyricsLine *newLines = (LyricsLine *)realloc(lyrics->lines, sizeof(LyricsLine) * capacity);
                                if (!newLines)
                                        return 0;
                                lyrics->lines = newLines;
                        }

                        char *start = text;
                        while (isspace((unsigned char)*start))
                                start++;
                        char *end = start + strlen(start);
                        while (end > start && isspace((unsigned char)*(end - 1)))
                                *(--end) = '\0';

                        lyrics->lines[lyrics->count].timestamp = min * 60.0 + sec + cs / 100.0;
                        lyrics->lines[lyrics->count].text = strdup(start);
                        if (!lyrics->lines[lyrics->count].text)
                                return 0;

                        lyrics->count++;
                }
        }

        lyrics->isTimed = 1;
        return 1;
}

Lyrics* getSyncedLyricsFromLIBLRC(SongData* songMetadata) {
    char* response;
    char* request = buildAPIRequest(*songMetadata);
    printf("%s\n", request);
    if (request == NULL) return NULL;

    response = curlRequest(request);
    if (response == NULL) return NULL;
    free(request);

    char* syncedLyrics = jsonParser(response, "syncedLyrics");
    free(response);
    if (syncedLyrics == NULL) return NULL;

    // in LIBLRC's response there are no actual newlines, but
    // a '\' followed by an 'n' instead
    char* fixedLyrics = str_replace(syncedLyrics, "\\n", "\n");
    free(syncedLyrics);
    if (fixedLyrics == NULL) return NULL;

    FILE* lyricsAsFile = fmemopen(fixedLyrics, strlen(fixedLyrics), "r");

    songMetadata->lyrics = (Lyrics *)calloc(1, sizeof(Lyrics));
    loadTimedLyrics(lyricsAsFile, songMetadata->lyrics);
    fclose(lyricsAsFile);
    free(fixedLyrics);

    return songMetadata->lyrics;
}

SongData initialiseSongData() {
    SongData metadata = {
        .filePath = {0},
        .coverArtPath = {0},
        .artist = NULL,
        .album = NULL,
        .title = NULL,
        .cover = NULL,
        .coverWidth = 0,
        .coverHeight = 0,
        .duration = 0.0f,
        .lyrics = NULL,
    };

    return metadata;
}

void freeLyrics(Lyrics *lyrics) {
        if (!lyrics)
                return;
        for (size_t i = 0; i < lyrics->count; i++)
                free(lyrics->lines[i].text);
        free(lyrics->lines);
        free(lyrics);
}

void deallocateSongData(SongData metadata) {
    free(metadata.artist);
    free(metadata.album);
    free(metadata.title);
    free(metadata.cover);
    freeLyrics(metadata.lyrics);
}
