/*
 * main.c: VOLT VIPER, for the SEGA Hikaru under Odin.
 *
 * A viper of light crawls on a grid. Each energy cell it eats makes it
 * longer; it must not hit a wall nor its own tail. A spark shows up now
 * and then, worth more while it lasts. Ten cells clear a level; eight
 * levels of walls, then they come round again, faster.
 *
 * Copyright (C) 2026 the odin-android contributors
 * GPL 3 or later, as Odin.
 */
#include "gfx.h"
#include "music.h"
#include "sound.h"
#include "sprites.h"

/* For the tests: the level a game starts at (build.py --start-wave) */
#ifndef START_WAVE
#define START_WAVE 1
#endif

/* ---- the field ----------------------------------------------------------- */

#define CELL     16
#define COLS     38
#define ROWS     24
#define FIELD_X  16
#define FIELD_Y  76
#define MAX_LEN  (COLS * ROWS)

#define LEVELS          8
#define CELLS_TO_CLEAR  10
#define START_LEN       4

/* Voices of the AICA */
enum { V_EAT, V_SPARK, V_TURN, V_CRASH, V_JINGLE };

#define RECORD_AT 0xF000u
#define RECORD_MAGIC 0x52495056u	/* "VPIR" */
#define SETTINGS_AT (RECORD_AT + 16u)
#define SETTINGS_MAGIC 0x53495056u	/* "VPIS" */

#define WHITE  rgb(240, 240, 250)
#define GREY   rgb(150, 150, 180)
#define YELLOW rgb(250, 220, 80)
#define RED    rgb(250, 80, 60)
#define CYAN   rgb(120, 230, 255)

enum { EMPTY, WALL, BODY };
enum { RIGHT, DOWN, LEFT, UP };
static const int dx[4] = { 1, 0, -1, 0 }, dy[4] = { 0, 1, 0, -1 };

typedef enum { TITLE, READY, PLAY, CRASH, CLEARED, OVER } state_t;

static struct {
	state_t state;
	unsigned timer, frame;
	unsigned score, hi, kept_hi, lives, level, round, next_life;

	uint8_t grid[ROWS][COLS];

	/* the viper: a ring of cells, the head at `head`, `len` long */
	uint8_t x[MAX_LEN], y[MAX_LEN];
	unsigned head, len, grow;
	int dir;
	int queue[2];
	unsigned queued;
	unsigned step, wait;		/* frames a step takes, frames to the next */

	int cell_x, cell_y;
	int spark_on, spark_x, spark_y;
	unsigned spark_time;
	unsigned eaten;
	unsigned banner;
} g;

static unsigned seed = 0x2468ACE1u;

static unsigned rnd(unsigned n)
{
	seed = seed * 1103515245u + 12345u;
	return (seed >> 16) % n;
}

static int pan_of(int col)
{
	const int p = (col - COLS / 2) / 3;
	return p < -7 ? -7 : p > 7 ? 7 : p;
}

/* ---- the best score, the music ------------------------------------------------ */

static void record_load(void)
{
	uint32_t r[3];

	backup_read(RECORD_AT, r, sizeof(r));
	if (r[0] == RECORD_MAGIC && (r[1] ^ 0x5A5A5A5Au) == r[2] && r[1] < 10000000u)
		g.hi = g.kept_hi = r[1];
	backup_read(SETTINGS_AT, r, 8);
	if (r[0] == SETTINGS_MAGIC)
		music_enable(r[1] != 0);
}

static void record_keep(void)
{
	if (g.hi <= g.kept_hi)
		return;
	const uint32_t r[3] = { RECORD_MAGIC, g.hi, g.hi ^ 0x5A5A5A5Au };
	backup_write(RECORD_AT, r, sizeof(r));
	g.kept_hi = g.hi;
}

static void settings_keep(void)
{
	const uint32_t r[2] = { SETTINGS_MAGIC, (uint32_t)music_enabled() };
	backup_write(SETTINGS_AT, r, sizeof(r));
}

/* ---- the walls of a level --------------------------------------------------------- */

static void wall(int c0, int r0, int c1, int r1)
{
	for (int r = r0; r <= r1; r++)
		for (int c = c0; c <= c1; c++)
			if (r >= 0 && r < ROWS && c >= 0 && c < COLS)
				g.grid[r][c] = WALL;
}

/* The viper starts on row 12 from column 4, going right: every level
 * leaves that clear */
static void level_walls(void)
{
	for (int r = 0; r < ROWS; r++)
		for (int c = 0; c < COLS; c++)
			g.grid[r][c] = EMPTY;
	switch ((g.level - 1) % LEVELS) {
	case 0:		/* the open field */
		break;
	case 1:		/* two bars */
		wall(10, 6, 27, 6);
		wall(10, 17, 27, 17);
		break;
	case 2:		/* four blocks */
		wall(6, 3, 9, 5);
		wall(28, 3, 31, 5);
		wall(6, 18, 9, 20);
		wall(28, 18, 31, 20);
		wall(17, 9, 20, 9);
		wall(17, 15, 20, 15);
		break;
	case 3:		/* the gates */
		wall(13, 0, 13, 8);
		wall(13, 16, 13, 23);
		wall(25, 0, 25, 8);
		wall(25, 16, 25, 23);
		break;
	case 4:		/* the cross */
		wall(18, 12, 31, 12);
		wall(24, 4, 24, 20);
		break;
	case 5:		/* the box with four doors */
		wall(8, 4, 29, 4);
		wall(8, 19, 29, 19);
		wall(8, 4, 8, 19);
		wall(29, 4, 29, 19);
		for (int r = 11; r <= 12; r++) {
			g.grid[r][8] = EMPTY;
			g.grid[r][29] = EMPTY;
		}
		for (int c = 18; c <= 19; c++) {
			g.grid[4][c] = EMPTY;
			g.grid[19][c] = EMPTY;
		}
		break;
	case 6:		/* the pillars */
		for (int c = 6; c < COLS - 2; c += 6)
			for (int r = 3; r < ROWS - 2; r += 6)
				if (r != 9 || c > 12)
					wall(c, r, c + 1, r + 1);
		break;
	case 7:		/* the switchbacks */
		wall(0, 4, 30, 4);
		wall(7, 8, 37, 8);
		wall(0, 16, 30, 16);
		wall(7, 20, 37, 20);
		break;
	}
}

/* ---- the viper ---------------------------------------------------------------------- */

static unsigned at(unsigned back)	/* the ring index `back` cells behind the head */
{
	return (g.head + MAX_LEN - back) % MAX_LEN;
}

static void viper_start(void)
{
	for (int r = 0; r < ROWS; r++)
		for (int c = 0; c < COLS; c++)
			if (g.grid[r][c] == BODY)
				g.grid[r][c] = EMPTY;
	g.len = START_LEN;
	g.head = START_LEN - 1;
	for (unsigned i = 0; i < START_LEN; i++) {
		g.x[i] = (uint8_t)(4 + i);
		g.y[i] = 12;
		g.grid[12][4 + i] = BODY;
	}
	g.grow = 0;
	g.dir = RIGHT;
	g.queued = 0;
	const unsigned faster = (g.level - 1) / 2 + g.round * 2;
	g.step = faster >= 4 ? 3 : 7 - faster;
	g.wait = g.step;
}

/* A cell no wall nor viper is on, away from the head */
static void free_cell(int *cx, int *cy)
{
	for (int tries = 0; tries < 2000; tries++) {
		const int c = (int)rnd(COLS), r = (int)rnd(ROWS);
		const int hx = g.x[g.head], hy = g.y[g.head];

		if (g.grid[r][c] != EMPTY || (c == g.cell_x && r == g.cell_y))
			continue;
		if ((c - hx) * (c - hx) + (r - hy) * (r - hy) < 16)
			continue;
		*cx = c;
		*cy = r;
		return;
	}
	*cx = 1;
	*cy = 1;
}

static void level_start(void)
{
	level_walls();
	g.cell_x = g.cell_y = -1;
	viper_start();
	free_cell(&g.cell_x, &g.cell_y);
	g.spark_on = 0;
	g.eaten = 0;
	g.banner = 150;
	g.state = READY;
	g.timer = 150;
	music_play(MUSIC_PLAY);
}

static void game_start(void)
{
	g.score = 0;
	g.lives = 3;
	g.level = START_WAVE;
	g.round = 0;
	g.next_life = 5000;
	sound_play(V_JINGLE, SND_START, 1024, 13, 0);
	level_start();
}

static void score(unsigned points)
{
	g.score += points;
	if (g.score >= g.next_life) {
		g.next_life += 7500;
		g.lives++;
		sound_play(V_JINGLE, SND_LIFE, 1024, 13, 0);
	}
}

/* The turns asked: a new direction pressed now, else the one held; none
 * back on itself, two kept ahead */
static void steer(uint16_t in, uint16_t pressed)
{
	static const uint16_t bits[4] = { IN_RIGHT, IN_DOWN, IN_LEFT, IN_UP };
	int want = -1;

	for (int d = 0; d < 4; d++)
		if (pressed & bits[d])
			want = d;
	if (want < 0)
		for (int d = 0; d < 4; d++)
			if (in & bits[d])
				want = d;
	if (want < 0 || g.queued >= 2)
		return;

	const int last = g.queued ? g.queue[g.queued - 1] : g.dir;

	if (want == last || want == (last + 2) % 4)
		return;
	g.queue[g.queued++] = want;
}

static void crash(void)
{
	g.state = CRASH;
	g.timer = 100;
	music_stop();
	sound_play(V_CRASH, SND_CRASH, 1024, 14, pan_of(g.x[g.head]));
}

static void move(void)
{
	if (g.queued) {
		g.dir = g.queue[0];
		g.queue[0] = g.queue[1];
		g.queued--;
		sound_play(V_TURN, SND_TURN, 1024, 6, pan_of(g.x[g.head]));
	}

	const int nx = g.x[g.head] + dx[g.dir], ny = g.y[g.head] + dy[g.dir];

	if (nx < 0 || nx >= COLS || ny < 0 || ny >= ROWS || g.grid[ny][nx] == WALL) {
		crash();
		return;
	}
	/* the tail moves on unless the viper grows: its cell is free */
	if (g.grow)
		g.grow--;
	else {
		const unsigned tail = at(g.len - 1);
		g.grid[g.y[tail]][g.x[tail]] = EMPTY;
		g.len--;
	}
	if (g.grid[ny][nx] == BODY) {
		g.len++;
		crash();
		return;
	}
	g.head = (g.head + 1) % MAX_LEN;
	g.x[g.head] = (uint8_t)nx;
	g.y[g.head] = (uint8_t)ny;
	g.len++;
	g.grid[ny][nx] = BODY;

	if (nx == g.cell_x && ny == g.cell_y) {
		g.grow += 3;
		g.eaten++;
		score(10 * g.level + 50 * g.round);
		sound_play(V_EAT, SND_EAT, 1024 + g.eaten * 40, 12, pan_of(nx));
		if (g.eaten >= CELLS_TO_CLEAR) {
			score(500 * g.level);
			g.state = CLEARED;
			g.timer = 150;
			music_stop();
			sound_play(V_JINGLE, SND_CLEARED, 1024, 13, 0);
			g.cell_x = g.cell_y = -1;
			g.spark_on = 0;
			return;
		}
		free_cell(&g.cell_x, &g.cell_y);
		if (!g.spark_on && rnd(3) == 0) {
			free_cell(&g.spark_x, &g.spark_y);
			g.spark_on = 1;
			g.spark_time = 360;
			sound_play(V_SPARK, SND_APPEAR, 1024, 9, pan_of(g.spark_x));
		}
	}
	if (g.spark_on && nx == g.spark_x && ny == g.spark_y) {
		g.spark_on = 0;
		g.grow += 1;
		score(100 * g.level + 100 * g.round);
		sound_play(V_SPARK, SND_SPARK, 1024, 12, pan_of(nx));
	}
}

static void play(uint16_t in, uint16_t pressed)
{
	steer(in, pressed);
	if (g.spark_on && --g.spark_time == 0)
		g.spark_on = 0;
	if (--g.wait == 0) {
		g.wait = g.step;
		move();
	}
}

/* ---- the drawing ------------------------------------------------------------------------- */

static uint16_t shade(unsigned r, unsigned g_, unsigned b, int f)	/* f in 16ths */
{
	return rgb(r * (unsigned)f / 16, g_ * (unsigned)f / 16, b * (unsigned)f / 16);
}

static void draw_centred(int y, const char *text, uint16_t colour, int scale)
{
	draw_text((SCREEN_W - text_width(text, scale)) / 2, y, text, colour, scale);
}

static void draw_walls(void)
{
	const uint16_t face = rgb(70, 60, 120), light = rgb(120, 110, 190), dark = rgb(35, 30, 70);

	/* the frame round the field */
	fill_rect(FIELD_X - 6, FIELD_Y - 6, COLS * CELL + 12, 6, face);
	fill_rect(FIELD_X - 6, FIELD_Y + ROWS * CELL, COLS * CELL + 12, 6, face);
	fill_rect(FIELD_X - 6, FIELD_Y, 6, ROWS * CELL, face);
	fill_rect(FIELD_X + COLS * CELL, FIELD_Y, 6, ROWS * CELL, face);
	for (int r = 0; r < ROWS; r++)
		for (int c = 0; c < COLS; c++)
			if (g.grid[r][c] == WALL) {
				const int x = FIELD_X + c * CELL, y = FIELD_Y + r * CELL;
				fill_rect(x, y, CELL, CELL, dark);
				fill_rect(x, y, CELL - 2, CELL - 2, face);
				fill_rect(x + 2, y + 2, CELL - 6, 2, light);
			}
}

static void draw_viper(int crashed)
{
	/* from the tail, so the head is drawn last; a glow that runs along */
	for (unsigned i = g.len; i-- > 0;) {
		const unsigned k = at(i);
		const int x = FIELD_X + g.x[k] * CELL, y = FIELD_Y + g.y[k] * CELL;
		int f = 16 - (int)(i * 8 / (g.len + 1));
		if (((g.frame / 3 + i) & 15) == 0)
			f = 16;
		const uint16_t body = crashed && (g.frame & 4) ? rgb(250, 90, 70) : shade(110, 230, 255, f);
		const uint16_t edge = crashed && (g.frame & 4) ? rgb(140, 40, 30) : shade(40, 120, 220, f);

		fill_rect(x + 1, y + 1, CELL - 2, CELL - 2, edge);
		fill_rect(x + 3, y + 3, CELL - 6, CELL - 6, body);
		/* joined to the cell ahead */
		if (i > 0) {
			const unsigned n = at(i - 1);
			const int ox = (int)g.x[n] - (int)g.x[k], oy = (int)g.y[n] - (int)g.y[k];
			if (ox)
				fill_rect(x + (ox > 0 ? CELL - 3 : -3), y + 3, 6, CELL - 6, body);
			if (oy)
				fill_rect(x + 3, y + (oy > 0 ? CELL - 3 : -3), CELL - 6, 6, body);
		}
	}
	/* the eyes of the head, looking where it goes */
	const int hx = FIELD_X + g.x[g.head] * CELL, hy = FIELD_Y + g.y[g.head] * CELL;
	const int ex = 6 + dx[g.dir] * 3, ey = 6 + dy[g.dir] * 3;
	const int sx = dy[g.dir] ? 3 : 0, sy = dx[g.dir] ? 3 : 0;
	fill_rect(hx + ex - sx, hy + ey - sy, 4, 4, WHITE);
	fill_rect(hx + ex + sx, hy + ey + sy, 4, 4, WHITE);
	fill_rect(hx + ex - sx + 1 + dx[g.dir], hy + ey - sy + 1 + dy[g.dir], 2, 2, rgb(10, 20, 40));
	fill_rect(hx + ex + sx + 1 + dx[g.dir], hy + ey + sy + 1 + dy[g.dir], 2, 2, rgb(10, 20, 40));
}

static void draw_scores(void)
{
	draw_text(12, 14, "SCORE", GREY, 2);
	draw_number(12, 34, g.score, 7, WHITE, 2);
	draw_text(SCREEN_W / 2 - 50, 14, "HI", GREY, 2);
	draw_number(SCREEN_W / 2 - 50, 34, g.hi, 7, YELLOW, 2);
	draw_text(SCREEN_W - 12 - text_width("LEVEL 00", 2), 14, "LEVEL", GREY, 2);
	draw_number(SCREEN_W - 12 - text_width("00", 2), 14, g.level, 2, WHITE, 2);
	if (g.state != TITLE) {
		/* the vipers left, and the cells still to eat */
		for (unsigned i = 0; i < g.lives && i < 8; i++)
			fill_rect(SCREEN_W - 12 - text_width("LEVEL 00", 2) + (int)i * 14, 38, 10, 10, CYAN);
		for (unsigned i = 0; i < CELLS_TO_CLEAR; i++)
			fill_rect(SCREEN_W / 2 + 70 + (int)i * 8, 40, 6, 8, i < g.eaten ? YELLOW : rgb(60, 60, 80));
	}
}

static void draw_field(void)
{
	clear(rgb(6, 8, 20));
	/* the dots of the grid */
	for (int r = 0; r <= ROWS; r += 2)
		for (int c = 0; c <= COLS; c += 2)
			fill_rect(FIELD_X + c * CELL - 1, FIELD_Y + r * CELL - 1, 2, 2, rgb(30, 34, 60));
	draw_walls();
	if (g.cell_x >= 0) {
		const int bob = (g.frame / 8) & 1;
		draw_sprite(&spr_cell, FIELD_X + g.cell_x * CELL, FIELD_Y + g.cell_y * CELL - bob);
	}
	if (g.spark_on && (g.spark_time > 90 || (g.frame & 4)))
		draw_sprite(&spr_spark, FIELD_X + g.spark_x * CELL, FIELD_Y + g.spark_y * CELL);
	draw_viper(g.state == CRASH);
}

static void draw_title(void)
{
	clear(rgb(6, 8, 20));
	/* a viper winding across the screen */
	for (int i = 0; i < 30; i++) {
		const int x = 24 + i * 20, y = 132 + ((i + (int)g.frame / 6) % 8 < 4 ? 0 : 10);
		const int f = 6 + (i * 10) / 30;
		fill_rect(x, y, 18, 14, shade(40, 120, 220, f));
		fill_rect(x + 2, y + 2, 14, 10, shade(110, 230, 255, f));
	}
	draw_centred(64, "VOLT VIPER", CYAN, 7);
	draw_centred(170, "EAT THE CELLS - MIND THE WALLS AND YOUR TAIL", GREY, 2);

	draw_sprite_scaled(&spr_cell, 180, 214, 2);
	draw_text(226, 222, "CELL: LONGER, POINTS", WHITE, 2);
	draw_sprite_scaled(&spr_spark, 180, 258, 2);
	draw_text(226, 266, "SPARK: MORE, FOR A WHILE", WHITE, 2);
	draw_centred(306, "10 CELLS CLEAR A LEVEL - 8 LEVELS OF WALLS", GREY, 2);

	if ((g.frame / 30) & 1)
		draw_centred(350, "PRESS START", WHITE, 3);
	draw_centred(408, music_enabled() ? "B / B2: MUSIC ON" : "B / B2: MUSIC OFF", GREY, 2);
	draw_centred(436, "STICK: STEER THE VIPER", GREY, 2);
}

/* ---- the loop ---------------------------------------------------------------------------- */

void main(void)
{
	uint16_t before = 0;

	layer_init();
	input_init();
	sound_init();
	record_load();
	g.state = TITLE;
	g.level = 1;

	for (;;) {
		g.frame++;
		seed += g.frame;
		const uint16_t in = input_read();
		const uint16_t pressed = (uint16_t)(in & ~before);
		before = in;

		switch (g.state) {
		case TITLE:
			music_play(MUSIC_TITLE);
			if (pressed & IN_B3 || pressed & IN_B2) {
				music_enable(!music_enabled());
				settings_keep();
			}
			if (pressed & IN_START)
				game_start();
			break;
		case READY:
			steer(in, pressed);
			if (--g.timer == 0)
				g.state = PLAY;
			break;
		case PLAY:
			play(in, pressed);
			break;
		case CRASH:
			if (--g.timer == 0) {
				if (--g.lives == 0) {
					g.state = OVER;
					g.timer = 240;
					sound_play(V_JINGLE, SND_OVER, 1024, 13, 0);
					record_keep();
				} else {
					viper_start();
					g.state = READY;
					g.timer = 60;
					music_play(MUSIC_PLAY);
				}
			}
			break;
		case CLEARED:
			if (--g.timer == 0) {
				g.level++;
				if ((g.level - 1) % LEVELS == 0)
					g.round++;
				record_keep();
				level_start();
			}
			break;
		case OVER:
			if (--g.timer == 0 || (g.timer < 180 && (pressed & IN_START)))
				g.state = TITLE;
			break;
		}
		if (g.score > g.hi)
			g.hi = g.score;
		sound_frame();
		music_frame();

		if (g.state == TITLE) {
			draw_title();
		} else {
			draw_field();
			if (g.state == READY && g.banner) {
				g.banner--;
				draw_centred(230, "LEVEL", WHITE, 4);
				draw_number(SCREEN_W / 2 - 18, 270, g.level, 2, YELLOW, 4);
			}
			if (g.state == READY && !g.banner)
				draw_centred(250, "READY", WHITE, 4);
			if (g.state == CLEARED)
				draw_centred(250, "LEVEL CLEARED", YELLOW, 4);
			if (g.state == OVER) {
				draw_centred(230, "GAME OVER", RED, 6);
				if (g.score && g.score == g.hi)
					draw_centred(300, "NEW RECORD!", YELLOW, 3);
			}
		}
		draw_scores();
		screen_flip();
	}
}
