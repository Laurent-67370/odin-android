/*
 * music.h: the music of the game, played by voices of the AICA (sound.h)
 * from the tracks of tools/music.py
 *
 * Copyright (C) 2026 the odin-android contributors
 * GPL 3 or later, as Odin.
 */
#ifndef MUSIC_H
#define MUSIC_H

#include "music_data.h"

/* The voices of the music: MUSIC_VOICE and the next ones, one a channel */
#define MUSIC_VOICE 16

/* Plays a track from its start, in a loop; the same track again goes on */
void music_play(unsigned track);
void music_stop(void);
/* On or off: off, nothing is heard, and music_play only remembers the track */
void music_enable(int on);
int music_enabled(void);
/* Once a frame */
void music_frame(void);

#endif
