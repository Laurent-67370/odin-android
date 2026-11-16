/*
 * main.c: the first homebrew of the Hikaru under Odin. A square moved by
 * the stick, coloured by the buttons; START changes the background.
 *
 * Copyright (C) 2026 the odin-android contributors
 * GPL 3 or later, as Odin.
 */
#include "hikaru.h"

#define SIZE 40

static const uint16_t *backgrounds(void)
{
	static uint16_t b[3];
	b[0] = rgb(20, 24, 48);
	b[1] = rgb(48, 20, 24);
	b[2] = rgb(16, 44, 24);
	return b;
}

void main(void)
{
	const uint16_t *bg = backgrounds();
	unsigned bg_n = 0;
	int x = (SCREEN_W - SIZE) / 2, y = (SCREEN_H - SIZE) / 2;
	uint16_t before = 0;

	layer_init();
	fill_rect(0, 0, SCREEN_W, SCREEN_H, bg[bg_n]);
	input_init();

	for (;;) {
		wait_frame();
		const uint16_t in = input_read();
		const uint16_t pressed = (uint16_t)(in & ~before);
		before = in;

		if (pressed & IN_START) {
			bg_n = (bg_n + 1) % 3;
			fill_rect(0, 0, SCREEN_W, SCREEN_H, bg[bg_n]);
		}

		const int speed = 4;
		int nx = x, ny = y;
		if (in & IN_LEFT)  nx -= speed;
		if (in & IN_RIGHT) nx += speed;
		if (in & IN_UP)    ny -= speed;
		if (in & IN_DOWN)  ny += speed;
		if (nx < 0) nx = 0;
		if (ny < 0) ny = 0;
		if (nx > SCREEN_W - SIZE) nx = SCREEN_W - SIZE;
		if (ny > SCREEN_H - SIZE) ny = SCREEN_H - SIZE;

		uint16_t colour = rgb(250, 250, 250);
		if (in & IN_B1) colour = rgb(250, 80, 60);
		if (in & IN_B2) colour = rgb(80, 220, 90);
		if (in & IN_B3) colour = rgb(70, 140, 250);

		if (nx != x || ny != y)
			fill_rect(x, y, SIZE, SIZE, bg[bg_n]);
		x = nx;
		y = ny;
		fill_rect(x, y, SIZE, SIZE, colour);
	}
}
