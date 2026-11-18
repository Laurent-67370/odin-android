/*
 * music.c: a sequencer, as music.h tells it
 *
 * Copyright (C) 2026 the odin-android contributors
 * GPL 3 or later, as Odin.
 *
 * Each channel of a track is a list of notes, each so many ticks long, a
 * tick so many frames. A note keys its voice on with the channel's
 * envelope; a rest lets it go (its release, RR).
 */
#include "music.h"
#include "sound.h"

#define MAX_CHANNELS 8

static int playing = -1;	/* the track, -1: none */
static int enabled = 1;
static unsigned frames_left;	/* in the tick */
static unsigned note_at[MAX_CHANNELS], ticks_left[MAX_CHANNELS];

static void silence(void)
{
	for (unsigned c = 0; c < MAX_CHANNELS; c++)
		sound_stop(MUSIC_VOICE + c);
}

static void start(unsigned track)
{
	playing = (int)track;
	frames_left = 0;
	for (unsigned c = 0; c < MAX_CHANNELS; c++) {
		note_at[c] = 0;
		ticks_left[c] = 0;
	}
}

void music_play(unsigned track)
{
	if (track >= MUSIC_COUNT || (int)track == playing)
		return;
	silence();
	start(track);
}

void music_stop(void)
{
	silence();
	playing = -1;
}

void music_enable(int on)
{
	enabled = on;
	if (!on)
		silence();
	else if (playing >= 0)
		start((unsigned)playing);
}

int music_enabled(void)
{
	return enabled;
}

void music_frame(void)
{
	if (playing < 0 || !enabled)
		return;
	if (frames_left) {
		frames_left--;
		return;
	}
	const mtrack_t *const t = &music_tracks[playing];
	frames_left = t->tick_frames - 1;
	for (unsigned c = 0; c < t->count && c < MAX_CHANNELS; c++) {
		const mchannel_t *const ch = &t->channels[c];
		if (ticks_left[c]) {
			ticks_left[c]--;
			continue;
		}
		const mnote_t *const n = &ch->notes[note_at[c]];
		note_at[c] = (note_at[c] + 1) % ch->count;
		ticks_left[c] = n->ticks ? n->ticks - 1u : 0;
		if (n->pitch)
			sound_play_env(MUSIC_VOICE + c, n->sound, n->pitch, ch->level, ch->pan, ch->rates, ch->decay);
		else
			sound_stop(MUSIC_VOICE + c);
	}
}
