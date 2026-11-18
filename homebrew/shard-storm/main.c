/*
 * main.c: SHARD STORM, a brick breaker for the SEGA Hikaru under Odin.
 *
 * A paddle at the bottom keeps a ball in play; the ball breaks the crystal
 * bricks of the level. Silver bricks take more hits, gold ones none, bombs
 * blow up their neighbours, and some bricks drop capsules: a wider paddle,
 * three balls, a laser, a slower ball, a sticky paddle, a fighter more.
 * Twelve levels, then they come round again, faster.
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

#define WALL_L    32
#define WALL_R    608
#define WALL_T    56
#define COLS      12
#define ROWS      12
#define CELL_W    48
#define CELL_H    22
#define BRICKS_Y  84

#define PADDLE_Y  436
#define PADDLE_H  12
#define PADDLE_W  80
#define WIDE_W    128

#define BALLS     6
#define BALL      8		/* its size, pixels */
#define FIX       256		/* positions and speeds in 256ths of a pixel */
#define BOLTS     4
#define CAPSULES  4

/* Voices of the AICA */
enum { V_BOUNCE, V_BRICK, V_CLANG, V_BOOM, V_LASER, V_CAPSULE, V_JINGLE, V_WALL };

#define RECORD_AT 0xF000u
#define RECORD_MAGIC 0x44524853u	/* "SHRD" */
#define SETTINGS_AT (RECORD_AT + 16u)
#define SETTINGS_MAGIC 0x53524853u	/* "SHRS" */

/* ---- the bricks ------------------------------------------------------------ */

/* A level as text: a row of twelve a line; '.' none, a colour letter a
 * brick of one hit, S silver (more hits), X gold (none), * a bomb, ? a
 * brick that always drops a capsule */
#define LEVELS 12
static const char *const levels[LEVELS][ROWS] = {
	{ /* 1: the bands */
		"............", "............", "wwwwwwwwwwww", "oooooooooooo", "yyyyyyyyyyyy", "gggggggggggg",
		"cccccccccccc", "bbbbbbbbbbbb", "............", "............", "............", "............" },
	{ /* 2: the pyramid */
		"............", ".....??.....", "....pppp....", "...rrrrrr...", "..oooooooo..", ".yyyyyyyyyy.",
		"gggggggggggg", "cc..cccc..cc", "............", "............", "............", "............" },
	{ /* 3: the checkers */
		"............", "S.S.S.S.S.S.", ".b.b.b.b.b.b", "g.g.g.g.g.g.", ".c.c.c.c.c.c", "y.y.y.y.y.y.",
		".o.o.o.o.o.o", "r.r.r.r.r.r.", ".p.p.p.p.p.p", "............", "............", "............" },
	{ /* 4: the frame */
		"............", "SSSSSSSSSSSS", "S..........S", "S.yyyyyyyy.S", "S.y?cccc?y.S", "S.y.cbbc.y.S",
		"S.y.cbbc.y.S", "S.yyyyyyyy.S", "S..........S", "SSSSS..SSSSS", "............", "............" },
	{ /* 5: the diamond */
		"............", ".....pp.....", "....pccp....", "...pcwwcp...", "..pcw**wcp..", ".pcw*??*wcp.",
		"..pcw**wcp..", "...pcwwcp...", "....pccp....", ".....pp.....", "............", "............" },
	{ /* 6: the columns */
		"............", "X.rr.XX.rr.X", "X.oo.XX.oo.X", "X.yy.??.yy.X", "X.gg.XX.gg.X", "X.cc.XX.cc.X",
		"X.bb.XX.bb.X", "X.pp.XX.pp.X", "............", "............", "............", "............" },
	{ /* 7: the minefield */
		"............", "gg*gg*gg*gg*", "cccccccccccc", "*bb*bb*bb*bb", "cccccccccccc", "gg*gg?gg*gg*",
		"yyyyyyyyyyyy", "............", "............", "............", "............", "............" },
	{ /* 8: the heart */
		"............", "..rr....rr..", ".rrrr..rrrr.", "rrwwrrrrrrrr", "rrwrrr?rrrrr", "rrrrrrrrrrrr",
		".rrrrrrrrrr.", "..rrrrrrrr..", "...rrrrrr...", "....rrrr....", ".....rr.....", "............" },
	{ /* 9: the zigzag */
		"............", "SS..........", "bbSS........", "..ccSS......", "....ggSS....", "......yySS..",
		"........ooSS", "......rrSS..", "....ppSS....", "..??SS......", "SSSS........", "............" },
	{ /* 10: the fortress */
		"............", "XXX.XXXX.XXX", "X.........?X", "X.SSSSSSSS.X", "X.S*cccc*S.X", "X.S.wwww.S.X",
		"X.S*cccc*S.X", "X.SSSSSSSS.X", "X..........X", "XXXXX..XXXXX", "............", "............" },
	{ /* 11: the spiral */
		"............", "pppppppppppp", "...........p", "bbbbbbbbbb.p", "b........b.p", "b.cccc?c.b.p",
		"b.c....c.b.p", "b.cccccc.b.p", "b........b.p", "bbbbbbbbbb.p", "............", "............" },
	{ /* 12: the storm */
		"............", "?S*SXSSXS*S?", "wwoowwoowwoo", "ggyyggyyggyy", "S*bcS*bcS*bc", "pprrpprrpprr",
		"XcXcXcXcXcXc", "yyyyyyyyyyyy", "*..*..*..*..", "............", "............", "............" },
};

enum { NONE, NORMAL, SILVER, GOLD, BOMB, BONUS };

typedef struct {
	uint8_t kind, hits, colour;
} brick_t;

/* The colours of the bricks, light, face, dark; their points */
static const struct {
	char letter;
	uint8_t r, g, b;
	uint16_t points;
} colours[] = {
	{ 'w', 235, 235, 245, 50 }, { 'o', 250, 150, 50, 60 }, { 'y', 245, 220, 70, 120 },
	{ 'g', 80, 220, 110, 80 }, { 'c', 70, 210, 235, 70 }, { 'b', 70, 120, 240, 100 },
	{ 'p', 220, 100, 240, 110 }, { 'r', 240, 70, 80, 90 },
	{ 'S', 175, 185, 200, 0 }, { 'X', 230, 180, 60, 0 }, { '*', 60, 60, 70, 100 }, { '?', 250, 250, 250, 200 },
};
#define N_COLOURS (sizeof(colours) / sizeof(colours[0]))

/* ---- the capsules -------------------------------------------------------- */

enum { CAP_WIDE, CAP_MULTI, CAP_LASER, CAP_SLOW, CAP_CATCH, CAP_LIFE, CAP_KINDS };
static const struct {
	char letter;
	uint8_t r, g, b;
} capsule_look[CAP_KINDS] = {
	{ 'W', 70, 120, 240 }, { 'M', 70, 210, 235 }, { 'L', 240, 70, 80 },
	{ 'S', 250, 150, 50 }, { 'C', 80, 220, 110 }, { '+', 220, 100, 240 },
};

/* ---- the game ------------------------------------------------------------- */

typedef enum { TITLE, SERVE, PLAY, LOST, CLEARED, OVER } state_t;

typedef struct {
	int on, x, y, vx, vy;	/* x, y: its top left corner, in FIX */
	int stuck, stuck_at;	/* on the paddle, at that offset */
} ball_t;

static struct {
	state_t state;
	unsigned timer, frame;
	unsigned score, hi, kept_hi, lives, level, round;
	unsigned next_life;		/* the score that gives a fighter more */

	brick_t bricks[ROWS][COLS];
	unsigned left;			/* the bricks still to break */

	int paddle_x, paddle_w, paddle_v;
	int laser, catching;
	unsigned slow_time;

	ball_t balls[BALLS];
	int speed;			/* of the balls, in FIX a frame */
	unsigned hits;			/* bricks hit since the serve: the ball speeds up */

	int bolt_on[BOLTS], bolt_x[BOLTS], bolt_y[BOLTS];
	int cap_on[CAPSULES], cap_x[CAPSULES], cap_y[CAPSULES], cap_kind[CAPSULES];

	int flash_x, flash_y;
	unsigned flash_time;
	unsigned banner;
} g;

static unsigned seed = 0x13572468u;

static unsigned rnd(unsigned n)
{
	seed = seed * 1103515245u + 12345u;
	return (seed >> 16) % n;
}

#define WHITE  rgb(240, 240, 250)
#define GREY   rgb(150, 150, 180)
#define YELLOW rgb(250, 220, 80)
#define RED    rgb(250, 80, 60)
#define CYAN   rgb(120, 230, 255)

/* sin and cos of the angles a ball leaves the paddle at, from -60 to 60
 * degrees off the vertical, in 256ths */
static const int16_t leave_sin[17] = { -222, -209, -195, -181, -165, -147, -128, -108, 0, 108, 128, 147, 165, 181, 195, 209, 222 };
static const int16_t leave_cos[17] = { 128, 147, 165, 181, 195, 209, 222, 232, 256, 232, 222, 209, 195, 181, 165, 147, 128 };

static int base_speed(void)
{
	/* 4 pixels a frame, a little more each level and each round */
	return 4 * FIX + (int)(g.level - 1) * 12 + (int)g.round * 96;
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

static int pan_of(int x)
{
	const int p = (x - SCREEN_W / 2) / 40;
	return p < -7 ? -7 : p > 7 ? 7 : p;
}

/* ---- a level ------------------------------------------------------------------ */

static void level_load(void)
{
	const char *const *map = levels[(g.level - 1) % LEVELS];

	g.left = 0;
	for (int r = 0; r < ROWS; r++)
		for (int c = 0; c < COLS; c++) {
			brick_t *const b = &g.bricks[r][c];
			const char ch = map[r][c];
			b->kind = NONE;
			b->hits = 0;
			for (unsigned k = 0; k < N_COLOURS; k++)
				if (colours[k].letter == ch)
					b->colour = (uint8_t)k;
			switch (ch) {
			case '.':
				continue;
			case 'S':
				b->kind = SILVER;
				b->hits = (uint8_t)(2 + (g.level - 1) / 4 + g.round);
				break;
			case 'X':
				b->kind = GOLD;
				continue;	/* never counted */
			case '*':
				b->kind = BOMB;
				b->hits = 1;
				break;
			case '?':
				b->kind = BONUS;
				b->hits = 1;
				break;
			default:
				b->kind = NORMAL;
				b->hits = 1;
			}
			g.left++;
		}
}

static void serve(void)
{
	for (int i = 0; i < BALLS; i++)
		g.balls[i].on = 0;
	ball_t *const b = &g.balls[0];
	b->on = 1;
	b->stuck = 1;
	b->stuck_at = g.paddle_w / 2 - BALL / 2 + 6;
	b->x = (g.paddle_x + b->stuck_at) * FIX;
	b->y = (PADDLE_Y - BALL) * FIX;
	g.speed = base_speed();
	g.hits = 0;
	for (int i = 0; i < BOLTS; i++)
		g.bolt_on[i] = 0;
	for (int i = 0; i < CAPSULES; i++)
		g.cap_on[i] = 0;
	g.state = SERVE;
	g.timer = 180;
}

static void level_start(void)
{
	level_load();
	g.paddle_w = PADDLE_W;
	g.paddle_x = (SCREEN_W - g.paddle_w) / 2;
	g.laser = g.catching = 0;
	g.slow_time = 0;
	g.banner = 150;
	music_play(((g.level - 1) & 1) ? MUSIC_LEVEL_B : MUSIC_LEVEL_A);
	serve();
}

static void game_start(void)
{
	g.score = 0;
	g.lives = 3;
	g.level = START_WAVE;
	g.round = 0;
	g.next_life = 20000;
	sound_play(V_JINGLE, SND_START, 1024, 13, 0);
	level_start();
}

/* ---- the ball and the bricks -------------------------------------------------- */

static void drop_capsule(int x, int y, int kind)
{
	for (int i = 0; i < CAPSULES; i++)
		if (!g.cap_on[i]) {
			g.cap_on[i] = 1;
			g.cap_x[i] = x;
			g.cap_y[i] = y;
			g.cap_kind[i] = kind;
			return;
		}
}

static void score(unsigned points)
{
	g.score += points;
	if (g.score >= g.next_life) {
		g.next_life += 30000;
		g.lives++;
		sound_play(V_CAPSULE, SND_LIFE, 1024, 13, 0);
	}
}

static void break_brick(int r, int c, int depth);

/* A brick hit (by a ball, a bolt, a bomb) */
static void hit_brick(int r, int c, int depth)
{
	if (r < 0 || r >= ROWS || c < 0 || c >= COLS)
		return;
	brick_t *const b = &g.bricks[r][c];
	const int x = WALL_L + c * CELL_W;

	if (b->kind == NONE)
		return;
	if (b->kind == GOLD) {
		sound_play(V_CLANG, SND_CLANG, 1400, 10, pan_of(x));
		return;
	}
	if (b->hits > 1) {
		b->hits--;
		sound_play(V_CLANG, SND_CLANG, 1024 + rnd(128), 11, pan_of(x));
		return;
	}
	break_brick(r, c, depth);
}

static void break_brick(int r, int c, int depth)
{
	brick_t *const b = &g.bricks[r][c];
	const int x = WALL_L + c * CELL_W, y = BRICKS_Y + r * CELL_H;
	const int kind = b->kind;

	b->kind = NONE;
	g.left--;
	g.hits++;
	if (kind == SILVER)
		score(50 * g.level);
	else
		score(colours[b->colour].points);
	/* the higher the brick, the higher its note */
	sound_play(V_BRICK, SND_CHIME, (unsigned)(1700 - r * 60 + (int)rnd(40)), 11, pan_of(x));
	if (kind == BONUS || (kind == NORMAL && rnd(9) == 0)) {
		int k = (int)rnd(CAP_KINDS);
		if (k == CAP_LIFE && rnd(3))	/* a fighter more is rarer */
			k = (int)rnd(CAP_LIFE);
		drop_capsule(x + CELL_W / 2 - 12, y, k);
	}
	if (kind == BOMB) {
		g.flash_x = x - CELL_W;
		g.flash_y = y - CELL_H;
		g.flash_time = 10;
		sound_play(V_BOOM, SND_BOMB, 1024, 13, pan_of(x));
		if (depth < 6)
			for (int dr = -1; dr <= 1; dr++)
				for (int dc = -1; dc <= 1; dc++)
					if ((dr || dc) && r + dr >= 0 && r + dr < ROWS && c + dc >= 0 && c + dc < COLS) {
						brick_t *const n = &g.bricks[r + dr][c + dc];
						/* a bomb breaks what is around it, gold too stays */
						if (n->kind != NONE && n->kind != GOLD) {
							n->hits = 1;
							break_brick(r + dr, c + dc, depth + 1);
						}
					}
	}
}

/* The brick under a box, if any: its row and column */
static int brick_at(int x, int y, int w, int h, int *row, int *col)
{
	const int c0 = (x - WALL_L) / CELL_W, c1 = (x + w - 1 - WALL_L) / CELL_W;
	const int r0 = (y - BRICKS_Y) / CELL_H, r1 = (y + h - 1 - BRICKS_Y) / CELL_H;

	if (y + h <= BRICKS_Y || y >= BRICKS_Y + ROWS * CELL_H)
		return 0;
	for (int r = r0; r <= r1; r++)
		for (int c = c0; c <= c1; c++)
			if (r >= 0 && r < ROWS && c >= 0 && c < COLS && g.bricks[r][c].kind != NONE) {
				*row = r;
				*col = c;
				return 1;
			}
	return 0;
}

static void ball_leave_paddle(ball_t *b)
{
	const int centre = b->x / FIX + BALL / 2 - g.paddle_x;
	int k = centre * 17 / g.paddle_w;
	if (k < 0) k = 0;
	if (k > 16) k = 16;
	/* never straight up: a little to the side it came from */
	if (k == 8)
		k = b->vx < 0 ? 7 : 9;
	b->vx = g.speed * leave_sin[k] / 256;
	b->vy = -g.speed * leave_cos[k] / 256;
}

/* A ball for a frame: in steps of at most 2 pixels, x then y, against the
 * walls, the bricks and the paddle */
static void ball_move(ball_t *b)
{
	if (b->stuck) {
		b->x = (g.paddle_x + b->stuck_at) * FIX;
		b->y = (PADDLE_Y - BALL) * FIX;
		return;
	}
	const int fast = b->vx < 0 ? -b->vx : b->vx;
	const int steps = 1 + (fast > (b->vy < 0 ? -b->vy : b->vy) ? fast : (b->vy < 0 ? -b->vy : b->vy)) / (2 * FIX);

	for (int s = 0; s < steps && b->on; s++) {
		int r, c;
		/* across */
		b->x += b->vx / steps;
		int x = b->x / FIX, y = b->y / FIX;
		if (x < WALL_L) {
			b->x = WALL_L * FIX;
			b->vx = -b->vx;
			sound_play(V_WALL, SND_WALL, 1024, 8, -6);
		} else if (x + BALL > WALL_R) {
			b->x = (WALL_R - BALL) * FIX;
			b->vx = -b->vx;
			sound_play(V_WALL, SND_WALL, 1024, 8, 6);
		} else if (brick_at(x, y, BALL, BALL, &r, &c)) {
			b->x -= b->vx / steps;
			b->vx = -b->vx;
			hit_brick(r, c, 0);
		}
		/* down or up */
		b->y += b->vy / steps;
		x = b->x / FIX;
		y = b->y / FIX;
		if (y < WALL_T) {
			b->y = WALL_T * FIX;
			b->vy = -b->vy;
			sound_play(V_WALL, SND_WALL, 1100, 8, pan_of(x));
		} else if (brick_at(x, y, BALL, BALL, &r, &c)) {
			b->y -= b->vy / steps;
			b->vy = -b->vy;
			hit_brick(r, c, 0);
		} else if (b->vy > 0 && y + BALL >= PADDLE_Y && y + BALL <= PADDLE_Y + PADDLE_H &&
		           x + BALL > g.paddle_x && x < g.paddle_x + g.paddle_w) {
			b->y = (PADDLE_Y - BALL) * FIX;
			if (g.catching) {
				b->stuck = 1;
				b->stuck_at = x - g.paddle_x;
				g.timer = 180;
			} else {
				ball_leave_paddle(b);
			}
			sound_play(V_BOUNCE, SND_BOUNCE, 1024, 11, pan_of(x));
		} else if (y > SCREEN_H) {
			b->on = 0;
		}
	}
}

static void release_balls(void)
{
	for (int i = 0; i < BALLS; i++) {
		ball_t *const b = &g.balls[i];
		if (b->on && b->stuck) {
			b->stuck = 0;
			ball_leave_paddle(b);
			sound_play(V_BOUNCE, SND_LAUNCH, 1024, 11, pan_of(b->x / FIX));
		}
	}
	if (g.state == SERVE)
		g.state = PLAY;
}

static void take_capsule(int kind)
{
	score(500);
	sound_play(V_CAPSULE, kind == CAP_LIFE ? SND_LIFE : SND_CAPSULE, 1024, 12, 0);
	switch (kind) {
	case CAP_WIDE: {
		const int centre = g.paddle_x + g.paddle_w / 2;
		g.paddle_w = WIDE_W;
		g.paddle_x = centre - WIDE_W / 2;
		break;
	}
	case CAP_MULTI: {
		/* each ball in play becomes three */
		for (int i = 0, n = 0; i < BALLS && n < BALLS; i++) {
			ball_t *const b = &g.balls[i];
			if (!b->on || b->stuck)
				continue;
			for (int k = 0; k < 2; k++)
				for (int j = 0; j < BALLS; j++)
					if (!g.balls[j].on) {
						g.balls[j] = *b;
						/* turned a little to each side */
						g.balls[j].vx = b->vx + (k ? 1 : -1) * g.speed / 3;
						g.balls[j].vy = b->vy < 0 ? b->vy : -b->vy;
						n++;
						break;
					}
		}
		release_balls();
		break;
	}
	case CAP_LASER:
		g.laser = 1;
		g.catching = 0;
		break;
	case CAP_SLOW:
		g.speed = base_speed() * 3 / 4;
		g.hits = 0;
		for (int i = 0; i < BALLS; i++) {
			ball_t *const b = &g.balls[i];
			b->vx = b->vx * 3 / 4;
			b->vy = b->vy * 3 / 4;
		}
		break;
	case CAP_CATCH:
		g.catching = 1;
		g.laser = 0;
		break;
	case CAP_LIFE:
		g.lives++;
		break;
	}
}

/* ---- a frame of play ------------------------------------------------------------ */

/* The sight's X axis (the finger on the phone, the mouse): the high byte of
 * analog channel 0, which the JVS program of the MIE keeps at Z80 E203 with
 * every poll (channel n at E203 + 2n), 0 on the left of the picture, FF on
 * the right. The stick and the keys work it too, back to the middle when
 * let go: it takes the paddle only when it moves with no stick held, and
 * not until the keys have let it settle. */
#define SIGHT_X (*(volatile uint8_t *)(0xA0820000u + 2u * 0xE203u))
#define SIGHT_STEP 2		/* a move it takes, in 256ths of the picture */
#define SIGHT_SETTLE 24		/* frames after the stick before it does */
#define SIGHT_FAST 48		/* the most the paddle goes in a frame after it */

static void follow_sight(int stick)
{
	static int last = -1, quiet, aimed;
	const int at = SIGHT_X;

	if (stick || last < 0) {
		quiet = 0;
		aimed = 0;
		last = at;
		return;
	}
	if (quiet < SIGHT_SETTLE) {
		quiet++;
		last = at;
		return;
	}
	if (at - last >= SIGHT_STEP || last - at >= SIGHT_STEP) {
		aimed = 1;
		last = at;
	}
	if (!aimed)
		return;
	/* the middle of the paddle under the finger */
	const int want = at * SCREEN_W / 255 - g.paddle_w / 2;
	int step = want - g.paddle_x;
	if (step > SIGHT_FAST)
		step = SIGHT_FAST;
	if (step < -SIGHT_FAST)
		step = -SIGHT_FAST;
	g.paddle_x += step;
}

static void play(uint16_t in, uint16_t pressed)
{
	/* the paddle: faster as the stick is held; or under the sight */
	const int dir = (in & IN_LEFT) ? -1 : (in & IN_RIGHT) ? 1 : 0;
	if (dir == 0)
		g.paddle_v = 0;
	else if (g.paddle_v * dir < 9)
		g.paddle_v = dir * (g.paddle_v * dir < 5 ? 5 : g.paddle_v * dir + 1);
	g.paddle_x += g.paddle_v;
	follow_sight(dir != 0);
	if (g.paddle_x < WALL_L)
		g.paddle_x = WALL_L;
	if (g.paddle_x > WALL_R - g.paddle_w)
		g.paddle_x = WALL_R - g.paddle_w;

	/* A: the ball off, or the laser */
	if (pressed & (IN_B1 | IN_B2)) {
		int any_stuck = 0;
		for (int i = 0; i < BALLS; i++)
			any_stuck |= g.balls[i].on && g.balls[i].stuck;
		if (any_stuck) {
			release_balls();
		} else if (g.laser) {
			for (int side = 0; side < 2; side++)
				for (int i = 0; i < BOLTS; i++)
					if (!g.bolt_on[i]) {
						g.bolt_on[i] = 1;
						g.bolt_x[i] = side ? g.paddle_x + g.paddle_w - 10 : g.paddle_x + 8;
						g.bolt_y[i] = PADDLE_Y - 10;
						break;
					}
			sound_play(V_LASER, SND_LASER, 1024, 10, pan_of(g.paddle_x));
		}
	}
	/* held too long on the paddle, the ball goes by itself */
	if (g.timer && --g.timer == 0)
		release_balls();

	/* the balls; they speed up as the bricks fall */
	const int target = base_speed() + (int)(g.hits > 60 ? 60 : g.hits) * 10;
	if (g.speed < target && (g.frame & 7) == 0)
		g.speed += 4;
	int in_play = 0;
	for (int i = 0; i < BALLS; i++) {
		ball_t *const b = &g.balls[i];
		if (!b->on)
			continue;
		ball_move(b);
		in_play += b->on;
	}
	if (!in_play) {
		g.state = LOST;
		g.timer = 100;
		sound_play(V_JINGLE, SND_LOSE, 1024, 13, 0);
		return;
	}

	/* the laser's bolts */
	for (int i = 0; i < BOLTS; i++) {
		if (!g.bolt_on[i])
			continue;
		g.bolt_y[i] -= 10;
		int r, c;
		if (g.bolt_y[i] < WALL_T) {
			g.bolt_on[i] = 0;
		} else if (brick_at(g.bolt_x[i], g.bolt_y[i], 2, 10, &r, &c)) {
			g.bolt_on[i] = 0;
			hit_brick(r, c, 0);
		}
	}

	/* the capsules */
	for (int i = 0; i < CAPSULES; i++) {
		if (!g.cap_on[i])
			continue;
		g.cap_y[i] += 2;
		if (g.cap_y[i] + 12 >= PADDLE_Y && g.cap_y[i] <= PADDLE_Y + PADDLE_H && g.cap_x[i] + 24 > g.paddle_x &&
		    g.cap_x[i] < g.paddle_x + g.paddle_w) {
			g.cap_on[i] = 0;
			take_capsule(g.cap_kind[i]);
		} else if (g.cap_y[i] > SCREEN_H) {
			g.cap_on[i] = 0;
		}
	}

	if (g.left == 0) {
		g.state = CLEARED;
		g.timer = 150;
		score(1000);
		music_stop();
		sound_play(V_JINGLE, SND_CLEARED, 1024, 13, 0);
	}
}

/* ---- drawing ---------------------------------------------------------------------- */

static uint16_t shade(unsigned r, unsigned g_, unsigned b, int f)	/* f in 16ths */
{
	const unsigned R = r * (unsigned)f / 16, G = g_ * (unsigned)f / 16, B = b * (unsigned)f / 16;
	return rgb(R > 255 ? 255 : R, G > 255 ? 255 : G, B > 255 ? 255 : B);
}

static void draw_centred(int y, const char *text, uint16_t colour, int scale)
{
	draw_text((SCREEN_W - text_width(text, scale)) / 2, y, text, colour, scale);
}

/* The background of a level: bands of a colour of its own, a faint grid */
static void draw_backdrop(void)
{
	static const uint8_t tints[6][3] = { { 20, 18, 60 }, { 50, 14, 40 }, { 10, 40, 50 },
	                                     { 40, 30, 10 }, { 14, 44, 24 }, { 40, 16, 60 } };
	const uint8_t *t = tints[(g.level - 1) % 6];
	for (int band = 0; band < 16; band++) {
		const int f = 6 + band / 2;
		fill_rect(0, band * 30, SCREEN_W, 30, shade(t[0], t[1], t[2], f));
	}
	const uint16_t line = shade(t[0], t[1], t[2], 18);
	for (int x = WALL_L; x < WALL_R; x += 48)
		fill_rect(x, WALL_T, 1, PADDLE_Y - WALL_T, line);
	/* the walls */
	fill_rect(WALL_L - 10, WALL_T - 10, 10, SCREEN_H - WALL_T + 10, rgb(120, 130, 160));
	fill_rect(WALL_R, WALL_T - 10, 10, SCREEN_H - WALL_T + 10, rgb(120, 130, 160));
	fill_rect(WALL_L - 10, WALL_T - 10, WALL_R - WALL_L + 20, 10, rgb(150, 160, 190));
	fill_rect(WALL_L - 8, WALL_T - 8, WALL_R - WALL_L + 16, 2, rgb(220, 225, 240));
}

static void draw_brick(int r, int c)
{
	const brick_t *const b = &g.bricks[r][c];
	const int x = WALL_L + c * CELL_W + 1, y = BRICKS_Y + r * CELL_H + 1, w = CELL_W - 2, h = CELL_H - 2;
	unsigned R = colours[b->colour].r, G = colours[b->colour].g, B = colours[b->colour].b;

	if (b->kind == BONUS) {	/* it shimmers */
		const unsigned k = (g.frame >> 2) % 3;
		R = k == 0 ? 250 : 120;
		G = k == 1 ? 250 : 140;
		B = k == 2 ? 250 : 200;
	}
	fill_rect(x, y, w, h, shade(R, G, B, 12));
	fill_rect(x, y, w, 2, shade(R, G, B, 20));
	fill_rect(x, y, 2, h, shade(R, G, B, 18));
	fill_rect(x, y + h - 2, w, 2, shade(R, G, B, 6));
	fill_rect(x + w - 2, y, 2, h, shade(R, G, B, 7));
	/* a facet: they are crystals */
	fill_rect(x + 6, y + 5, 10, 2, shade(R, G, B, 22));
	if (b->kind == BOMB) {
		fill_rect(x + w / 2 - 5, y + 5, 10, 10, rgb(240, 80, 50));
		fill_rect(x + w / 2 - 1, y + 3, 2, 3, YELLOW);
	} else if (b->kind == SILVER && b->hits > 1) {
		draw_number(x + w / 2 - 3, y + 6, b->hits, 1, rgb(60, 60, 80), 1);
	} else if (b->kind == GOLD) {
		fill_rect(x + 4, y + h / 2 - 1, w - 8, 2, shade(R, G, B, 17));
	}
}

static void draw_paddle(void)
{
	const int x = g.paddle_x, w = g.paddle_w;
	const uint16_t body = g.laser ? rgb(200, 70, 80) : g.catching ? rgb(70, 190, 100) : rgb(160, 170, 200);

	fill_rect(x + 6, PADDLE_Y, w - 12, PADDLE_H, body);
	fill_rect(x + 6, PADDLE_Y, w - 12, 3, rgb(240, 240, 250));
	fill_rect(x + 6, PADDLE_Y + PADDLE_H - 3, w - 12, 3, rgb(70, 70, 100));
	/* glowing ends */
	const uint16_t glow = (g.frame >> 3) & 1 ? CYAN : rgb(80, 180, 230);
	fill_rect(x, PADDLE_Y + 1, 6, PADDLE_H - 2, glow);
	fill_rect(x + w - 6, PADDLE_Y + 1, 6, PADDLE_H - 2, glow);
	if (g.laser) {
		fill_rect(x + 7, PADDLE_Y - 6, 4, 6, RED);
		fill_rect(x + w - 11, PADDLE_Y - 6, 4, 6, RED);
	}
}

static void draw_field(void)
{
	draw_backdrop();
	for (int r = 0; r < ROWS; r++)
		for (int c = 0; c < COLS; c++)
			if (g.bricks[r][c].kind != NONE)
				draw_brick(r, c);
	if (g.flash_time) {
		g.flash_time--;
		fill_rect(g.flash_x, g.flash_y, CELL_W * 3, CELL_H * 3, (g.flash_time & 2) ? YELLOW : RED);
	}
	for (int i = 0; i < CAPSULES; i++)
		if (g.cap_on[i]) {
			const int k = g.cap_kind[i];
			const uint16_t col = rgb(capsule_look[k].r, capsule_look[k].g, capsule_look[k].b);
			fill_rect(g.cap_x[i] + 2, g.cap_y[i], 20, 12, col);
			fill_rect(g.cap_x[i], g.cap_y[i] + 2, 24, 8, col);
			fill_rect(g.cap_x[i] + 2, g.cap_y[i] + 1, 20, 2, WHITE);
			const char text[2] = { capsule_look[k].letter, 0 };
			draw_text(g.cap_x[i] + 9, g.cap_y[i] + 3, text, rgb(20, 20, 30), 1);
		}
	for (int i = 0; i < BOLTS; i++)
		if (g.bolt_on[i])
			fill_rect(g.bolt_x[i], g.bolt_y[i], 2, 10, rgb(255, 120, 120));
	if (g.state != LOST && g.state != OVER)
		draw_paddle();
	for (int i = 0; i < BALLS; i++)
		if (g.balls[i].on)
			draw_sprite(&spr_ball, g.balls[i].x / FIX, g.balls[i].y / FIX);
	/* the paddles left */
	for (unsigned l = 1; l < g.lives && l < 8; l++) {
		fill_rect(WALL_L + (int)l * 34 - 30, SCREEN_H - 14, 28, 6, rgb(160, 170, 200));
		fill_rect(WALL_L + (int)l * 34 - 30, SCREEN_H - 14, 4, 6, CYAN);
		fill_rect(WALL_L + (int)l * 34 - 6, SCREEN_H - 14, 4, 6, CYAN);
	}
}

static void draw_scores(void)
{
	draw_text(12, 14, "SCORE", GREY, 2);
	draw_number(12 + text_width("SCORE ", 2), 14, g.score, 7, WHITE, 2);
	draw_text(SCREEN_W / 2 - 50, 14, "HI", GREY, 2);
	draw_number(SCREEN_W / 2 - 50 + text_width("HI ", 2), 14, g.hi, 7, YELLOW, 2);
	draw_text(SCREEN_W - 12 - text_width("LEVEL 00", 2), 14, "LEVEL", GREY, 2);
	draw_number(SCREEN_W - 12 - text_width("00", 2), 14, g.level, 2, WHITE, 2);
}

static void draw_title(void)
{
	static const uint8_t tint[3] = { 16, 14, 50 };
	for (int band = 0; band < 16; band++)
		fill_rect(0, band * 30, SCREEN_W, 30, shade(tint[0], tint[1], tint[2], 6 + band / 2));
	draw_sprite_scaled(&spr_shard, 120, 60, 4);
	draw_sprite_scaled(&spr_shard, SCREEN_W - 120 - 64, 60, 4);
	draw_centred(72, "SHARD", CYAN, 7);
	draw_centred(130, "STORM", rgb(230, 120, 250), 7);
	draw_centred(196, "BREAK EVERY CRYSTAL", GREY, 2);

	for (int k = 0; k < CAP_KINDS; k++) {
		static const char *const names[CAP_KINDS] = { "WIDE", "MULTI", "LASER", "SLOW", "CATCH", "LIFE" };
		const int x = 70 + (k % 3) * 180, y = 240 + (k / 3) * 32;
		const uint16_t col = rgb(capsule_look[k].r, capsule_look[k].g, capsule_look[k].b);
		fill_rect(x + 2, y, 20, 12, col);
		fill_rect(x, y + 2, 24, 8, col);
		const char text[2] = { capsule_look[k].letter, 0 };
		draw_text(x + 9, y + 3, text, rgb(20, 20, 30), 1);
		draw_text(x + 34, y, names[k], WHITE, 2);
	}
	draw_centred(318, "SILVER: MORE HITS   GOLD: NONE   BOMBS BLOW UP", rgb(250, 180, 90), 1);
	if (!input_ready())
		draw_centred(350, "WAITING FOR THE I/O BOARD...", RED, 2);
	else if ((g.frame >> 5) & 1)
		draw_centred(345, "PRESS START", WHITE, 3);
	draw_centred(408, music_enabled() ? "B / B2: MUSIC ON" : "B / B2: MUSIC OFF", GREY, 2);
	draw_centred(436, "FINGER OR STICK: MOVE   TAP / A: LAUNCH, LASER", GREY, 2);
}

/* ---- the loop ---------------------------------------------------------------------- */

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
		case SERVE:
		case PLAY:
			play(in, pressed);
			break;
		case LOST:
			if (--g.timer == 0) {
				if (--g.lives == 0) {
					g.state = OVER;
					g.timer = 240;
					music_stop();
					sound_play(V_JINGLE, SND_OVER, 1024, 13, 0);
					record_keep();
				} else {
					g.paddle_w = PADDLE_W;
					g.laser = g.catching = 0;
					serve();
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
			if (g.state == SERVE && g.banner) {
				g.banner--;
				draw_centred(300, "LEVEL", WHITE, 4);
				draw_number(SCREEN_W / 2 - 18, 340, g.level, 2, YELLOW, 4);
			}
			if (g.state == CLEARED)
				draw_centred(300, "LEVEL CLEARED", YELLOW, 4);
			if (g.state == OVER) {
				draw_centred(260, "GAME OVER", RED, 6);
				if (g.score && g.score == g.hi)
					draw_centred(330, "NEW RECORD!", YELLOW, 3);
			}
		}
		draw_scores();
		screen_flip();
	}
}
