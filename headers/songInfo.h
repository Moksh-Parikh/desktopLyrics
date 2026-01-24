// All 3 of these structs, loadTimedLyrics() and freeLyrics()
// are taken/adapted from the kew codebase
// https://codeberg.org/ravachol/kew
// Credit to ravachol and the other kew contributors

#pragma once

#include <limits.h>

typedef struct {
        double timestamp;
        char *text;
} LyricsLine;

typedef struct {
        LyricsLine *lines;
        size_t count;
        int max_length;
        int isTimed;
} Lyrics;

typedef struct {
        char filePath[PATH_MAX];
        char coverArtPath[PATH_MAX];
        char* artist;
        char* album;
        char* title;
        unsigned char *cover;
        int coverWidth;
        int coverHeight;
        double duration;
        Lyrics *lyrics;
} SongData;

int loadTimedLyrics(char* inBuffer, Lyrics *lyrics);
Lyrics* getSyncedLyricsFromLIBLRC(SongData* songMetadata);
double* generateLyricTimeArray(Lyrics* lyrics);
SongData initialiseSongData();
void freeLyrics(Lyrics *lyrics);
void deallocateSongData(SongData metadata);
