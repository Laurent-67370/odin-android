/*
 * main.c: NOVA SIEGE, a fixed shooter for the SEGA Hikaru under Odin.
 *
 * A formation marches from side to side and down, faster as it thins out;
 * the fighter at the bottom shoots it, two shots at a time, from behind four
 * shields the bombs and the shots wear away. A raider crosses the top now
 * and then. Three fighters, a new wave lower each time the formation is
 * cleared.
 *
 * Copyright (C) 2026 the odin-android contributors
 * GPL 3 or later, as Odin.
 */
#include "gfx.h"
#include "sprites.h"

#define Z 2			/* the sprites twice their size */

/* ---- the field ----------------------------------------------------------- */

#define TOP       44		/* below the scores */
#define GROUND    452		/* the line under the fighter */
#define FIGHTER_Y (GROUND - 8 * Z - 6)

#define COLS      10
#define ROWS      5
#define CELL_W    44
#define CELL_H    36
#define ALIEN_W   (12 * Z)
#define ALIEN_H   (8 * Z)

#define SHIELDS   4
#define SHIELD_W  24		/* cells of Z x Z pixels */
#define SHIELD_H  16
#define SHIELD_Y  (FIGHTER_Y - 60)

#define BOMBS     4
#define SHOTS     2		/* the fighter's shots in the air at once */
#define SHOT_SPEED 12
#define FIRE_KEPT 12		/* frames a press waits for a free shot */

typedef enum { TITLE, PLAY, DYING, CLEARED, OVER } state_t;

static struct {
	state_t state;
	unsigned timer, frame;
	unsigned score, hi, lives, wave;

	/* the formation */
	uint8_t alive[ROWS][COLS];
	int left;		/* its first column's x */
	int top;		/* its first row's y */
	int dir;		/* +1, -1 */
	unsigned count, step_wait, pose;

	/* the fighter and its shots; a press kept while none is free */
	int x;
	int shot_on[SHOTS], shot_x[SHOTS], shot_y[SHOTS];
	unsigned fire_kept;

	int bomb_on[BOMBS], bomb_x[BOMBS], bomb_y[BOMBS];

	int raider_on, raider_x, raider_dir;
	unsigned raider_wait;

	/* what blew up, shown a few frames */
	int boom_x, boom_y;
	unsigned boom_time;
	const sprite_t *boom;
	unsigned bonus, bonus_time;
	int bonus_x;

	uint8_t shield[SHIELDS][SHIELD_H][SHIELD_W];
} g;

static unsigned seed = 0x2468ACE1u;

static unsigned rnd(unsigned n)
{
	seed = seed * 1103515245u + 12345u;
	return (seed >> 16) % n;
}

/* ---- the colours ---------------------------------------------------------- */

#define SKY       rgb(6, 6, 18)
#define GREEN     rgb(110, 230, 90)
#define WHITE     rgb(240, 240, 250)
#define GREY      rgb(150, 150, 180)
#define YELLOW    rgb(250, 220, 80)
#define RED       rgb(250, 80, 60)

/* ---- the formation -------------------------------------------------------- */

static const sprite_t *alien_sprite(int row, unsigned pose)
{
	if (row == 0)
		return pose ? &spr_watcher_1 : &spr_watcher_0;
	if (row < 3)
		return pose ? &spr_jelly_1 : &spr_jelly_0;
	return pose ? &spr_beetle_1 : &spr_beetle_0;
}

static unsigned alien_points(int row)
{
	return row == 0 ? 30 : row < 3 ? 20 : 10;
}

static int alien_x(int c)
{
	return g.left + c * CELL_W + (CELL_W - ALIEN_W) / 2;
}

static int alien_y(int r)
{
	return g.top + r * CELL_H;
}

/* The wait between two steps: from 30 frames for a full formation down to
 * 1 for the last one */
static unsigned step_wait_of(unsigned count)
{
	return 1 + (count * 29) / (ROWS * COLS);
}

/* ---- the shields ------------------------------------------------------------ */

static int shield_x(int s)
{
	const int gap = (SCREEN_W - SHIELDS * SHIELD_W * Z) / (SHIELDS + 1);
	return gap + s * (SHIELD_W * Z + gap);
}

static void shields_build(void)
{
	for (int s = 0; s < SHIELDS; s++)
		for (int y = 0; y < SHIELD_H; y++)
			for (int x = 0; x < SHIELD_W; x++) {
				/* a block with its top corners cut, an arch below */
				const int dx = x < SHIELD_W / 2 ? SHIELD_W / 2 - 1 - x : x - SHIELD_W / 2;
				int on = 1;
				if (y < 4 && dx > SHIELD_W / 2 - 1 - (4 - y))
					on = 0;
				if (y > SHIELD_H - 6 && dx < 5)
					on = 0;
				g.shield[s][y][x] = (uint8_t)on;
			}
}

/* Whether a point hits a shield; if so, a bite is taken around it */
static int shield_hit(int px, int py)
{
	if (py < SHIELD_Y || py >= SHIELD_Y + SHIELD_H * Z)
		return 0;
	for (int s = 0; s < SHIELDS; s++) {
		const int x0 = shield_x(s);
		if (px < x0 || px >= x0 + SHIELD_W * Z)
			continue;
		const int cx = (px - x0) / Z, cy = (py - SHIELD_Y) / Z;
		if (!g.shield[s][cy][cx])
			return 0;
		for (int dy = -2; dy <= 2; dy++)
			for (int dx = -2; dx <= 2; dx++) {
				const int x = cx + dx, y = cy + dy;
				if (x < 0 || y < 0 || x >= SHIELD_W || y >= SHIELD_H)
					continue;
				if (dx * dx + dy * dy <= 2 || rnd(3) == 0)
					g.shield[s][y][x] = 0;
			}
		return 1;
	}
	return 0;
}

/* The formation reaching the shields wipes out what it covers */
static void shields_trample(void)
{
	for (int r = ROWS - 1; r >= 0; r--)
		for (int c = 0; c < COLS; c++) {
			if (!g.alive[r][c])
				continue;
			const int ay = alien_y(r) + ALIEN_H;
			if (ay < SHIELD_Y)
				continue;
			for (int s = 0; s < SHIELDS; s++)
				for (int y = 0; y < SHIELD_H && SHIELD_Y + y * Z < ay; y++)
					for (int x = 0; x < SHIELD_W; x++) {
						const int px = shield_x(s) + x * Z;
						if (px + Z > alien_x(c) && px < alien_x(c) + ALIEN_W)
							g.shield[s][y][x] = 0;
					}
		}
}

/* ---- the game ------------------------------------------------------------------ */

static void wave_start(void)
{
	for (int r = 0; r < ROWS; r++)
		for (int c = 0; c < COLS; c++)
			g.alive[r][c] = 1;
	g.count = ROWS * COLS;
	g.left = (SCREEN_W - COLS * CELL_W) / 2;
	/* each wave a little lower, up to five */
	g.top = TOP + 40 + (int)((g.wave - 1) % 6) * 14;
	g.dir = 1;
	g.step_wait = step_wait_of(g.count);
	g.pose = 0;
	for (int i = 0; i < SHOTS; i++)
		g.shot_on[i] = 0;
	g.fire_kept = 0;
	for (int b = 0; b < BOMBS; b++)
		g.bomb_on[b] = 0;
	g.raider_on = 0;
	g.raider_wait = 900 + rnd(600);
	shields_build();
}

static void game_start(void)
{
	g.score = 0;
	g.lives = 3;
	g.wave = 1;
	g.x = (SCREEN_W - 15 * Z) / 2;
	wave_start();
	g.state = PLAY;
}

static void explode(const sprite_t *what, int x, int y)
{
	g.boom = what;
	g.boom_x = x;
	g.boom_y = y;
	g.boom_time = 16;
}

static void formation_step(void)
{
	/* the columns still standing at each side */
	int first = COLS, last = -1, bottom = -1;
	for (int c = 0; c < COLS; c++)
		for (int r = 0; r < ROWS; r++)
			if (g.alive[r][c]) {
				if (c < first) first = c;
				if (c > last) last = c;
				if (r > bottom) bottom = r;
			}
	if (last < 0)
		return;

	const int step = 6;
	const int lx = alien_x(first) + g.dir * step;
	const int rx = alien_x(last) + ALIEN_W + g.dir * step;
	if (lx < 8 || rx > SCREEN_W - 8) {
		g.dir = -g.dir;
		g.top += 12;
	} else {
		g.left += g.dir * step;
	}
	g.pose ^= 1;
	shields_trample();
	/* down to the fighter: the planet is lost */
	if (alien_y(bottom) + ALIEN_H >= FIGHTER_Y) {
		g.lives = 0;
		explode(&spr_blast, g.x - Z, FIGHTER_Y);
		g.state = DYING;
		g.timer = 120;
	}
}

static void bombs_drop(void)
{
	/* more bombs as the waves go */
	const unsigned chance = g.wave < 5 ? 40 - g.wave * 5 : 15;
	if (rnd(chance))
		return;
	int b = 0;
	while (b < BOMBS && g.bomb_on[b])
		b++;
	if (b == BOMBS)
		return;
	/* the lowest alien of a column, the one over the fighter more often */
	int c = (int)rnd(COLS);
	if (rnd(3) == 0) {
		const int over = (g.x + 15 * Z / 2 - g.left) / CELL_W;
		if (over >= 0 && over < COLS)
			c = over;
	}
	for (int r = ROWS - 1; r >= 0; r--)
		if (g.alive[r][c]) {
			g.bomb_on[b] = 1;
			g.bomb_x[b] = alien_x(c) + ALIEN_W / 2 - 1;
			g.bomb_y[b] = alien_y(r) + ALIEN_H;
			return;
		}
}

static void fighter_hit(void)
{
	explode(&spr_blast, g.x - Z, FIGHTER_Y);
	g.state = DYING;
	g.timer = 100;
	for (int i = 0; i < SHOTS; i++)
		g.shot_on[i] = 0;
	g.fire_kept = 0;
	for (int b = 0; b < BOMBS; b++)
		g.bomb_on[b] = 0;
}

static void play(uint16_t in, uint16_t pressed)
{
	/* the fighter */
	if (in & IN_LEFT)
		g.x -= 4;
	if (in & IN_RIGHT)
		g.x += 4;
	if (g.x < 8)
		g.x = 8;
	if (g.x > SCREEN_W - 8 - 15 * Z)
		g.x = SCREEN_W - 8 - 15 * Z;
	/* a press fires as soon as a shot is free, within FIRE_KEPT frames */
	if (pressed & (IN_B1 | IN_B2))
		g.fire_kept = FIRE_KEPT;
	if (g.fire_kept) {
		g.fire_kept--;
		for (int i = 0; i < SHOTS; i++)
			if (!g.shot_on[i]) {
				g.shot_on[i] = 1;
				g.shot_x[i] = g.x + 7 * Z;
				g.shot_y[i] = FIGHTER_Y - 8;
				g.fire_kept = 0;
				break;
			}
	}

	/* the shots */
	for (int i = 0; i < SHOTS; i++) {
		if (!g.shot_on[i])
			continue;
		g.shot_y[i] -= SHOT_SPEED;
		const int sx = g.shot_x[i], sy = g.shot_y[i];
		if (sy < TOP || shield_hit(sx, sy) || shield_hit(sx, sy + 8)) {
			g.shot_on[i] = 0;
			continue;
		}
		for (int r = 0; r < ROWS && g.shot_on[i]; r++)
			for (int c = 0; c < COLS; c++) {
				if (!g.alive[r][c])
					continue;
				const int ax = alien_x(c), ay = alien_y(r);
				/* from where the shot was to where it is: none slips through */
				if (sx + 2 > ax && sx < ax + ALIEN_W && sy < ay + ALIEN_H && sy + 10 + SHOT_SPEED > ay) {
					g.alive[r][c] = 0;
					g.count--;
					g.score += alien_points(r);
					g.shot_on[i] = 0;
					explode(&spr_boom, ax, ay);
					g.step_wait = step_wait_of(g.count);
					break;
				}
			}
		if (g.shot_on[i] && g.raider_on && sy < TOP + 7 * Z + 4 && sx >= g.raider_x && sx < g.raider_x + 16 * Z) {
			static const unsigned bonuses[4] = { 50, 100, 150, 300 };
			g.bonus = bonuses[rnd(4)];
			g.score += g.bonus;
			g.bonus_x = g.raider_x;
			g.bonus_time = 90;
			g.raider_on = 0;
			g.shot_on[i] = 0;
			explode(&spr_blast, g.raider_x, TOP + 4);
		}
	}

	/* the formation */
	if (g.timer == 0) {
		formation_step();
		g.timer = g.step_wait;
	} else {
		g.timer--;
	}
	if (g.state != PLAY)
		return;
	if (g.count == 0) {
		g.state = CLEARED;
		g.timer = 120;
		return;
	}

	/* the bombs */
	bombs_drop();
	for (int b = 0; b < BOMBS; b++) {
		if (!g.bomb_on[b])
			continue;
		g.bomb_y[b] += 4 + (g.wave > 3);
		if (g.bomb_y[b] > GROUND || shield_hit(g.bomb_x[b], g.bomb_y[b] + 10)) {
			g.bomb_on[b] = 0;
			continue;
		}
		/* a bomb and a shot meet: both gone */
		int met = 0;
		for (int i = 0; i < SHOTS && !met; i++)
			if (g.shot_on[i] && g.bomb_x[b] + 3 > g.shot_x[i] - 2 && g.bomb_x[b] < g.shot_x[i] + 4 &&
			    g.bomb_y[b] + 10 > g.shot_y[i] && g.bomb_y[b] < g.shot_y[i] + 10) {
				g.bomb_on[b] = g.shot_on[i] = 0;
				met = 1;
			}
		if (met)
			continue;
		if (g.bomb_y[b] + 10 > FIGHTER_Y + 2 && g.bomb_x[b] + 3 > g.x + 2 && g.bomb_x[b] < g.x + 15 * Z - 2) {
			fighter_hit();
			return;
		}
	}

	/* the raider */
	if (g.raider_on) {
		g.raider_x += g.raider_dir * 2;
		if (g.raider_x < -16 * Z || g.raider_x > SCREEN_W)
			g.raider_on = 0;
	} else if (g.raider_wait == 0) {
		g.raider_on = 1;
		g.raider_dir = rnd(2) ? 1 : -1;
		g.raider_x = g.raider_dir > 0 ? -16 * Z : SCREEN_W;
		g.raider_wait = 1200 + rnd(900);
	} else {
		g.raider_wait--;
	}
}

/* ---- drawing ---------------------------------------------------------------------- */

static void draw_centred(int y, const char *text, uint16_t colour, int scale)
{
	draw_text((SCREEN_W - text_width(text, scale)) / 2, y, text, colour, scale);
}

static void draw_scores(void)
{
	draw_text(12, 10, "SCORE", GREY, 2);
	draw_number(12 + text_width("SCORE ", 2), 10, g.score, 5, WHITE, 2);
	draw_text(SCREEN_W / 2 - text_width("HI 00000", 2) / 2, 10, "HI", GREY, 2);
	draw_number(SCREEN_W / 2 - text_width("HI 00000", 2) / 2 + text_width("HI ", 2), 10, g.hi, 5, YELLOW, 2);
	draw_text(SCREEN_W - 12 - text_width("WAVE 00", 2), 10, "WAVE", GREY, 2);
	draw_number(SCREEN_W - 12 - text_width("00", 2), 10, g.wave, 2, WHITE, 2);
}

static void draw_field(void)
{
	/* the shields */
	for (int s = 0; s < SHIELDS; s++)
		for (int y = 0; y < SHIELD_H; y++)
			for (int x = 0; x < SHIELD_W; x++)
				if (g.shield[s][y][x])
					fill_rect(shield_x(s) + x * Z, SHIELD_Y + y * Z, Z, Z, GREEN);
	/* the formation */
	for (int r = 0; r < ROWS; r++)
		for (int c = 0; c < COLS; c++)
			if (g.alive[r][c])
				draw_sprite_scaled(alien_sprite(r, g.pose), alien_x(c), alien_y(r), Z);
	if (g.raider_on)
		draw_sprite_scaled(&spr_raider, g.raider_x, TOP + 4, Z);
	if (g.bonus_time) {
		g.bonus_time--;
		draw_number(g.bonus_x + 4, TOP + 6, g.bonus, 3, RED, 2);
	}
	if (g.boom_time) {
		g.boom_time--;
		draw_sprite_scaled(g.boom, g.boom_x, g.boom_y, Z);
	}
	/* the shots, the bombs */
	for (int i = 0; i < SHOTS; i++)
		if (g.shot_on[i])
			fill_rect(g.shot_x[i], g.shot_y[i], 2, 10, WHITE);
	for (int b = 0; b < BOMBS; b++)
		if (g.bomb_on[b]) {
			const int wiggle = ((g.bomb_y[b] >> 3) & 1) ? 2 : 0;
			fill_rect(g.bomb_x[b] + wiggle, g.bomb_y[b], 2, 4, YELLOW);
			fill_rect(g.bomb_x[b] + 2 - wiggle, g.bomb_y[b] + 4, 2, 4, YELLOW);
			fill_rect(g.bomb_x[b] + wiggle, g.bomb_y[b] + 8, 2, 3, YELLOW);
		}
	/* the fighter, unless it is the one exploding */
	if (g.state != DYING && g.state != OVER)
		draw_sprite_scaled(&spr_fighter, g.x, FIGHTER_Y, Z);
	/* the ground, the fighters left */
	fill_rect(0, GROUND, SCREEN_W, 2, GREEN);
	draw_number(12, GROUND + 8, g.lives, 1, WHITE, 2);
	for (unsigned l = 1; l < g.lives && l < 6; l++)
		draw_sprite(&spr_fighter, 30 + (int)l * 22, GROUND + 10);
}

static void draw_title(void)
{
	draw_centred(80, "NOVA SIEGE", YELLOW, 7);
	draw_centred(150, "HOLD THE LINE", GREY, 2);

	const sprite_t *table[4] = { &spr_raider, &spr_watcher_0, &spr_jelly_0, &spr_beetle_0 };
	const char *points[4] = { "= ? MYSTERY", "= 30 POINTS", "= 20 POINTS", "= 10 POINTS" };
	for (int i = 0; i < 4; i++) {
		const int y = 200 + i * 40;
		draw_sprite_scaled(table[i], 220 - table[i]->w * Z / 2, y, Z);
		draw_text(260, y + 2, points[i], WHITE, 2);
	}
	if (!input_ready())
		draw_centred(380, "WAITING FOR THE I/O BOARD...", RED, 2);
	else if ((g.frame >> 5) & 1)
		draw_centred(380, "PRESS START", WHITE, 3);
	draw_centred(440, "STICK: MOVE   A / B1: FIRE", GREY, 2);
}

/* ---- the loop ---------------------------------------------------------------------- */

void main(void)
{
	uint16_t before = 0;

	layer_init();
	input_init();
	g.state = TITLE;
	g.wave = 1;
	g.lives = 3;
	wave_start();

	for (;;) {
		g.frame++;
		seed += g.frame;
		const uint16_t in = input_read();
		const uint16_t pressed = (uint16_t)(in & ~before);
		before = in;

		switch (g.state) {
		case TITLE:
			if (pressed & IN_START)
				game_start();
			break;
		case PLAY:
			play(in, pressed);
			break;
		case DYING:
			if (--g.timer == 0) {
				if (g.lives > 0)
					g.lives--;
				if (g.lives == 0) {
					g.state = OVER;
					g.timer = 240;
				} else {
					g.state = PLAY;
					g.x = (SCREEN_W - 15 * Z) / 2;
				}
			}
			break;
		case CLEARED:
			if (--g.timer == 0) {
				g.wave++;
				wave_start();
				g.state = PLAY;
			}
			break;
		case OVER:
			if (--g.timer == 0 || (g.timer < 180 && (pressed & IN_START)))
				g.state = TITLE;
			break;
		}
		if (g.score > g.hi)
			g.hi = g.score;

		clear(SKY);
		draw_scores();
		if (g.state == TITLE) {
			draw_title();
		} else {
			draw_field();
			if (g.state == CLEARED)
				draw_centred(SCREEN_H / 2 - 20, "WAVE CLEARED", YELLOW, 4);
			if (g.state == OVER)
				draw_centred(SCREEN_H / 2 - 40, "GAME OVER", RED, 6);
		}
		screen_flip();
	}
}
