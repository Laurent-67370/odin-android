/*
 * main.c: the demo of the homebrew. A ship moved by the stick fires at
 * aliens (button 1); START changes the colours of the stars.
 *
 * Copyright (C) 2026 the odin-android contributors
 * GPL 3 or later, as Odin.
 */
#include "gfx.h"
#include "sprites.h"

#define STARS   48
#define SHOTS   6
#define ALIENS  6
#define ZOOM    2	/* the sprites twice as large */

typedef struct { int x, y, on; } thing_t;

static unsigned seed = 12345;
static unsigned rnd(unsigned n)
{
	seed = seed * 1103515245u + 12345u;
	return (seed >> 16) % n;
}

static int hits(const thing_t *a, int aw, int ah, const thing_t *b, int bw, int bh)
{
	return a->x < b->x + bw && b->x < a->x + aw && a->y < b->y + bh && b->y < a->y + ah;
}

void main(void)
{
	thing_t stars[STARS], shots[SHOTS], aliens[ALIENS];
	thing_t ship = { (SCREEN_W - 16 * ZOOM) / 2, SCREEN_H - 80, 1 };
	const uint16_t star_colours[3] = { rgb(200, 200, 230), rgb(250, 200, 120), rgb(140, 250, 200) };
	unsigned palette = 0, score = 0, frame = 0;
	uint16_t before = 0;

	layer_init();
	input_init();
	for (int i = 0; i < STARS; i++)
		stars[i] = (thing_t){ (int)rnd(SCREEN_W), (int)rnd(SCREEN_H), 1 + (int)rnd(3) };
	for (int i = 0; i < SHOTS; i++)
		shots[i].on = 0;
	for (int i = 0; i < ALIENS; i++)
		aliens[i] = (thing_t){ 30 + i * 100, 70 + (i & 1) * 40, 1 };

	for (;;) {
		frame++;
		const uint16_t in = input_read();
		const uint16_t pressed = (uint16_t)(in & ~before);
		before = in;

		/* Moving */
		if (pressed & IN_START)
			palette = (palette + 1) % 3;
		if (in & IN_LEFT)  ship.x -= 4;
		if (in & IN_RIGHT) ship.x += 4;
		if (in & IN_UP)    ship.y -= 4;
		if (in & IN_DOWN)  ship.y += 4;
		if (ship.x < 0) ship.x = 0;
		if (ship.x > SCREEN_W - spr_ship.w * ZOOM) ship.x = SCREEN_W - spr_ship.w * ZOOM;
		if (ship.y < 40) ship.y = 40;
		if (ship.y > SCREEN_H - 24 - spr_ship.h * ZOOM) ship.y = SCREEN_H - 24 - spr_ship.h * ZOOM;
		if (pressed & IN_B1)
			for (int i = 0; i < SHOTS; i++)
				if (!shots[i].on) {
					shots[i] = (thing_t){ ship.x + 8 * ZOOM - 2, ship.y - 8, 1 };
					break;
				}
		for (int i = 0; i < STARS; i++) {
			stars[i].y += stars[i].on;
			if (stars[i].y >= SCREEN_H) {
				stars[i].y = 0;
				stars[i].x = (int)rnd(SCREEN_W);
			}
		}
		for (int i = 0; i < SHOTS; i++)
			if (shots[i].on && (shots[i].y -= 8) < 30)
				shots[i].on = 0;
		for (int i = 0; i < ALIENS; i++) {
			thing_t *a = &aliens[i];
			if (!a->on) {
				if (rnd(120) == 0)
					*a = (thing_t){ (int)rnd(SCREEN_W - 16 * ZOOM), 40, 1 };
				continue;
			}
			a->x += ((frame / 64 + (unsigned)i) & 1) ? 1 : -1;
			if (a->x < 0)
				a->x = 0;
			if (a->x > SCREEN_W - spr_alien.w * ZOOM)
				a->x = SCREEN_W - spr_alien.w * ZOOM;
			if ((frame & 3) == 0)
				a->y++;
			if (a->y > SCREEN_H)
				a->on = 0;
			for (int j = 0; j < SHOTS; j++)
				if (shots[j].on && hits(&shots[j], 4, 10, a, spr_alien.w * ZOOM, spr_alien.h * ZOOM)) {
					shots[j].on = a->on = 0;
					score += 10;
				}
		}

		/* Drawing */
		clear(rgb(8, 8, 24));
		for (int i = 0; i < STARS; i++)
			fill_rect(stars[i].x, stars[i].y, stars[i].on > 2 ? 2 : 1, stars[i].on > 2 ? 2 : 1,
			          star_colours[palette]);
		for (int i = 0; i < ALIENS; i++)
			if (aliens[i].on)
				draw_sprite_scaled(&spr_alien, aliens[i].x, aliens[i].y, ZOOM);
		for (int i = 0; i < SHOTS; i++)
			if (shots[i].on)
				fill_rect(shots[i].x, shots[i].y, 4, 10, rgb(250, 240, 120));
		draw_sprite_scaled(&spr_ship, ship.x, ship.y, ZOOM);

		draw_text(12, 10, "ODIN HOMEBREW", rgb(250, 200, 80), 2);
		draw_text(SCREEN_W - 12 - text_width("SCORE 00000", 2), 10, "SCORE", rgb(200, 200, 220), 2);
		draw_number(SCREEN_W - 12 - text_width("00000", 2), 10, score, 5, rgb(250, 250, 250), 2);
		if (!input_ready())
			draw_text((SCREEN_W - text_width("WAITING FOR THE I/O BOARD...", 2)) / 2, SCREEN_H / 2,
			          "WAITING FOR THE I/O BOARD...", rgb(250, 120, 90), 2);
		else
			draw_text(12, SCREEN_H - 22, "STICK: MOVE  B1: FIRE  START: STARS",
			          rgb(150, 150, 190), 2);
		screen_flip();
	}
}
