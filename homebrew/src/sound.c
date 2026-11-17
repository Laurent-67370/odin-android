/*
 * sound.c: the AICA, as sound.h tells it
 *
 * Copyright (C) 2026 the odin-android contributors
 * GPL 3 or later, as Odin.
 *
 * The board is on the external bus, page 0C; the window at 02000000 is
 * opened on it by the memory controller (byte 1A of its registers). Its
 * registers are at 700000 of the page, its wave memory at 800000. The
 * registers of a voice are those of the Dreamcast's AICA: 0x80 bytes, a
 * 16-bit value every 4 bytes.
 */
#include "sound.h"

#define WINDOW_PAGE (*(volatile uint8_t *)0xA400001Au)
#define AICA(off)   (*(volatile uint32_t *)(0xA2700000u + (off)))
#define WAVE        ((volatile uint32_t *)0xA2800000u)

/* Where the samples start in the wave memory, clear of what the BOOTROM
 * left */
#define SAMPLES_AT 0x10000u

enum {
	PLAY = 0x00, SA = 0x04, LSA = 0x08, LEA = 0x0C, RATES = 0x10, DECAY = 0x14,
	PITCH = 0x18, LFO = 0x1C, EFFECT = 0x20, DIRECT = 0x24, LEVEL = 0x28,
};
#define KYONEX 0x8000u
#define KYONB  0x4000u
#define LPCTL  0x0200u
#define LPOFF  0x0020u		/* no filter */

static uint32_t sample_at[SOUND_COUNT];

/* KYONEX keys every voice on or off as its KYONB is: a sound that ended
 * with its KYONB still set would start again. Each voice played once has
 * KYONB cleared a little before its end (frames_left), without KYONEX. */
static unsigned frames_left[64];

static void slot(unsigned voice, unsigned reg, uint32_t value)
{
	AICA(voice * 0x80u + reg) = value & 0xFFFFu;
}

void sound_init(void)
{
	WINDOW_PAGE = 0x0C;
	AICA(0x2C00) = AICA(0x2C00) | 1u;	/* the ARM7 in reset: no driver */
	for (unsigned v = 0; v < 64; v++) {
		slot(v, PLAY, 0);
		slot(v, PLAY, KYONEX);		/* every voice released */
	}
	AICA(0x2800) = 0x000F;			/* MVOL at the top */

	/* the samples, two at a time */
	uint32_t at = SAMPLES_AT;
	for (unsigned s = 0; s < SOUND_COUNT; s++) {
		const sound_t *const snd = &sounds[s];
		sample_at[s] = at;
		volatile uint32_t *to = WAVE + at / 4;
		for (unsigned i = 0; i + 1 < snd->length; i += 2)
			*to++ = (uint16_t)snd->data[i] | (uint32_t)(uint16_t)snd->data[i + 1] << 16;
		at += (snd->length * 2 + 3) & ~3u;
	}
}

void sound_frame(void)
{
	for (unsigned v = 0; v < 64; v++)
		if (frames_left[v] && --frames_left[v] == 0)
			slot(v, PLAY, AICA(v * 0x80u + PLAY) & ~KYONB);
}

void sound_stop(unsigned voice)
{
	frames_left[voice] = 0;
	slot(voice, PLAY, AICA(voice * 0x80u + PLAY) & ~KYONB);
	slot(voice, PLAY, (AICA(voice * 0x80u + PLAY) & ~KYONB) | KYONEX);
}

void sound_play(unsigned voice, unsigned sound, unsigned pitch, unsigned level, int pan)
{
	if (sound >= SOUND_COUNT || voice >= 64)
		return;
	const sound_t *const snd = &sounds[sound];
	const uint32_t at = sample_at[sound];

	/* released first: a voice still sounding does not start again */
	sound_stop(voice);

	/* the pitch: OCT -1 plays the samples at their own 22050 Hz; each
	 * octave up or down, then FNS for the rest */
	int oct = -1;
	unsigned p = pitch ? pitch : 1024;
	while (p >= 2048) { p >>= 1; oct++; }
	while (p < 1024) { p <<= 1; oct--; }
	const unsigned fns = p - 1024;

	const unsigned loop = snd->loop_start >= 0;
	slot(voice, SA, at & 0xFFFF);
	slot(voice, LSA, loop ? (unsigned)snd->loop_start : 0);
	slot(voice, LEA, snd->length - 1);
	slot(voice, RATES, 0x001F);		/* AR at once, no decay */
	slot(voice, DECAY, 0x001F);		/* a quick release */
	slot(voice, PITCH, ((unsigned)(oct & 0xF) << 11) | (fns & 0x3FF));
	slot(voice, LFO, 0);
	slot(voice, EFFECT, 0);
	const unsigned dipan = pan < 0 ? 0x10u | (unsigned)(-pan > 15 ? 15 : -pan) : (unsigned)(pan > 15 ? 15 : pan);
	slot(voice, DIRECT, ((level > 15 ? 15 : level) << 8) | dipan);
	slot(voice, LEVEL, LPOFF);		/* TL 0: full level */
	const uint32_t play = (loop ? LPCTL : 0) | ((at >> 16) & 0x7F);
	slot(voice, PLAY, play | KYONB);
	slot(voice, PLAY, play | KYONB | KYONEX);
	/* its frames at 60 a second, rounded down; a loop keeps KYONB */
	frames_left[voice] = loop ? 0 : (unsigned)((snd->length * 60u * 1024u / 22050u) / (pitch ? pitch : 1024));
	if (!loop && frames_left[voice] == 0)
		frames_left[voice] = 1;
}
