/*
 * gfx.c: drawing, as gfx.h tells it
 *
 * Copyright (C) 2026 the odin-android contributors
 * GPL 3 or later, as Odin.
 *
 * A word of the framebuffer RAM holds two pixels, swapped: the pixel x is
 * at x ^ 1 of its line.
 */
#include "gfx.h"
#include "font5x7.h"

void clear(uint16_t colour)
{
	const uint32_t pair = (uint32_t)colour << 16 | colour;

	for (int y = 0; y < SCREEN_H; y++) {
		volatile uint32_t *line = (volatile uint32_t *)(screen + y * SCREEN_PITCH);
		for (int x = 0; x < SCREEN_W / 2; x += 4) {
			line[x] = pair;
			line[x + 1] = pair;
			line[x + 2] = pair;
			line[x + 3] = pair;
		}
	}
}

void fill_rect(int x, int y, int w, int h, uint16_t colour)
{
	if (x < 0) { w += x; x = 0; }
	if (y < 0) { h += y; y = 0; }
	if (x + w > SCREEN_W) w = SCREEN_W - x;
	if (y + h > SCREEN_H) h = SCREEN_H - y;
	if (w <= 0 || h <= 0)
		return;
	/* Even spans by 32-bit words: two pixels of one colour, whatever their
	 * order in the word */
	const uint32_t pair = (uint32_t)colour << 16 | colour;
	for (int j = 0; j < h; j++) {
		volatile uint16_t *line = screen + (y + j) * SCREEN_PITCH;
		int i = 0;
		if ((x & 1) && w > 0) {
			line[x ^ 1] = colour;
			i = 1;
		}
		volatile uint32_t *words = (volatile uint32_t *)(line + x + i);
		for (; i + 1 < w; i += 2)
			*words++ = pair;
		if (i < w)
			line[(x + i) ^ 1] = colour;
	}
}

void draw_sprite(const sprite_t *s, int x, int y)
{
	for (int j = 0; j < s->h; j++) {
		const int sy = y + j;
		if (sy < 0 || sy >= SCREEN_H)
			continue;
		volatile uint16_t *line = screen + sy * SCREEN_PITCH;
		const uint16_t *from = s->pixels + j * s->w;
		for (int i = 0; i < s->w; i++) {
			const int sx = x + i;
			if (from[i] && sx >= 0 && sx < SCREEN_W)
				line[sx ^ 1] = from[i];
		}
	}
}

void draw_sprite_scaled(const sprite_t *s, int x, int y, int scale)
{
	if (scale <= 1) {
		draw_sprite(s, x, y);
		return;
	}
	for (int j = 0; j < s->h; j++)
		for (int i = 0; i < s->w; i++) {
			const uint16_t p = s->pixels[j * s->w + i];
			if (p)
				fill_rect(x + i * scale, y + j * scale, scale, scale, p);
		}
}

static void draw_char(int x, int y, char c, uint16_t colour, int scale)
{
	if (c >= 'a' && c <= 'z')
		c = (char)(c - 'a' + 'A');
	if (c < FONT_FIRST || c > FONT_LAST)
		c = '?';
	const uint8_t *rows = font5x7[c - FONT_FIRST];
	for (int r = 0; r < 7; r++)
		for (int b = 0; b < 5; b++)
			if (rows[r] & (0x10 >> b))
				fill_rect(x + b * scale, y + r * scale, scale, scale, colour);
}

void draw_text(int x, int y, const char *text, uint16_t colour, int scale)
{
	for (; *text; text++, x += 6 * scale)
		if (*text != ' ')
			draw_char(x, y, *text, colour, scale);
}

int text_width(const char *text, int scale)
{
	int n = 0;
	while (text[n])
		n++;
	return n ? (n * 6 - 1) * scale : 0;
}

void draw_number(int x, int y, unsigned value, int digits, uint16_t colour, int scale)
{
	char text[12];

	if (digits > 10)
		digits = 10;
	text[digits] = 0;
	for (int i = digits - 1; i >= 0; i--) {
		text[i] = (char)('0' + value % 10);
		value /= 10;
	}
	draw_text(x, y, text, colour, scale);
}
