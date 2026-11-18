/*
 * sound.h: the AICA of the main sound board, played from the SH-4: the
 * samples of build/gen/sounds.c in its wave memory, a voice for each sound
 *
 * Copyright (C) 2026 the odin-android contributors
 * GPL 3 or later, as Odin.
 */
#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>
#include "sounds.h"

/* The board opened through a window of the bus, its ARM7 held in reset,
 * the samples copied to its memory */
void sound_init(void);

/* Plays a sound on a voice (0 to 63), at `pitch` 1024ths of its own speed,
 * `level` 0 (silent) to 15, `pan` -15 (left) to 15 (right); a sound that
 * loops plays until sound_stop */
void sound_play(unsigned voice, unsigned sound, unsigned pitch, unsigned level, int pan);
void sound_stop(unsigned voice);
/* The same with an envelope: `rates` and `decay` as the AICA's registers 10
 * (D2R, D1R, AR) and 14 (KRS, DL, RR) of a voice; sound_play uses 0x1F and
 * 0x1F, at once, no decay, a quick release */
void sound_play_env(unsigned voice, unsigned sound, unsigned pitch, unsigned level, int pan, unsigned rates,
                    unsigned decay);
/* Once a frame: the voices whose sound ended let go */
void sound_frame(void);

#endif
