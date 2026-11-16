/*
 * gfx.h: drawing in the picture of layer 1 (hikaru.h): rectangles, sprites,
 * text
 *
 * Copyright (C) 2026 the odin-android contributors
 * GPL 3 or later, as Odin.
 */
#ifndef GFX_H
#define GFX_H

#include "hikaru.h"

/* An image of ABGR1555 pixels; 0 is transparent */
typedef struct {
	int w, h;
	const uint16_t *pixels;
} sprite_t;

void clear(uint16_t colour);
void fill_rect(int x, int y, int w, int h, uint16_t colour);
void draw_sprite(const sprite_t *s, int x, int y);
/* `scale` times as large, each pixel a square */
void draw_sprite_scaled(const sprite_t *s, int x, int y, int scale);

/* The 5x7 font of tools/font5x7.txt, `scale` times as large; a character
 * takes 6 x 8 pixels at scale 1. Lowercase is drawn as uppercase. */
void draw_text(int x, int y, const char *text, uint16_t colour, int scale);
int text_width(const char *text, int scale);
/* A number, right-aligned on `digits` places with zeros */
void draw_number(int x, int y, unsigned value, int digits, uint16_t colour, int scale);

#endif
