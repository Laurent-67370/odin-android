/*
 * main.c: NOVA SIEGE, a fixed shooter for the SEGA Hikaru under Odin.
 *
 * A formation marches from side to side and down, faster as it thins out;
 * the fighter at the bottom shoots it, two shots at a time, from behind four
 * shields the bombs and the shots wear away. A raider crosses the top now
 * and then. The waves change: the shape of the formation, armoured aliens
 * that take two hits, divers that leave it to swoop on the fighter, and a
 * mothership every fifth wave. Sound and music by the AICA; the best score
 * and the choice of music kept in the backup RAM.
 *
 * Copyright (C) 2026 the odin-android contributors
 * GPL 3 or later, as Odin.
 */
#include "gfx.h"
#include "music.h"
#include "sound.h"
#include "sprites.h"

#define Z 2			/* the sprites twice their size */

/* For the tests: the wave a game starts at (build.py --start-wave) */
#ifndef START_WAVE
#define START_WAVE 1
#endif

/* ---- the field ----------------------------------------------------------- */

#define TOP       44		/* below the scores */
#define GROUND    452		/* the line under the fighter */
#define FIGHTER_W (15 * Z)
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

#define BOMBS     6
#define SHOTS     2		/* the fighter's shots in the air at once */
#define SHOT_SPEED 12
#define FIRE_KEPT 12		/* frames a press waits for a free shot */
#define DIVERS    3

#define BOSS_Z    3
#define BOSS_W    (32 * BOSS_Z)
#define BOSS_H    (13 * BOSS_Z)

/* The voices of the AICA, one a kind of sound */
enum { V_SHOT, V_BOOM, V_BLAST, V_STEP, V_SIREN, V_JINGLE, V_CLANG, V_SWOOP };

/* Where the best score is kept in the backup RAM */
#define RECORD_AT 0xF000u
#define RECORD_MAGIC 0x4753564Eu	/* "NVSG" */
#define SETTINGS_AT (RECORD_AT + 16u)
#define SETTINGS_MAGIC 0x5353564Eu	/* "NVSS" */

typedef enum { TITLE, PLAY, DYING, CLEARED, OVER } state_t;

typedef struct {
	int on, r, c;		/* its place in the formation */
	int x, y, t;
	int back;		/* gone past the ground, flying back to its place */
	int hp, armoured;
} diver_t;

static struct {
	state_t state;
	unsigned timer, frame;
	unsigned score, hi, kept_hi, lives, wave;
	unsigned banner;	/* frames the name of the wave is shown */

	/* the formation: the hits each alien still takes (0: none there) */
	uint8_t hp[ROWS][COLS];
	uint8_t armoured[ROWS][COLS];
	int left;		/* its first column's x */
	int top;		/* its first row's y */
	int dir;		/* +1, -1 */
	unsigned count, step_wait, pose, steps;
	unsigned dive_wait;
	diver_t divers[DIVERS];

	/* the mothership */
	int boss_on, boss_x, boss_y, boss_dir, boss_hp, boss_max;
	unsigned boss_fire, boss_flash;

	/* the fighter and its shots; a press kept while none is free */
	int x;
	int shot_on[SHOTS], shot_x[SHOTS], shot_y[SHOTS];
	unsigned fire_kept;

	int bomb_on[BOMBS], bomb_x[BOMBS], bomb_y[BOMBS], bomb_dx[BOMBS];

	int raider_on, raider_x, raider_dir;
	unsigned raider_wait;

	/* what blew up, shown a few frames */
	int boom_x, boom_y, boom_z;
	unsigned boom_time;
	const sprite_t *boom;
	unsigned bonus, bonus_time;
	int bonus_x, bonus_y;

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
#define ORANGE    rgb(250, 150, 40)

/* ---- the sound -------------------------------------------------------------- */

/* Left to right as the screen, gently */
static int pan_of(int x)
{
	const int p = (x - SCREEN_W / 2) / 40;
	return p < -7 ? -7 : p > 7 ? 7 : p;
}

/* ---- the best score ---------------------------------------------------------- */

static void record_load(void)
{
	uint32_t r[3];

	backup_read(RECORD_AT, r, sizeof(r));
	if (r[0] == RECORD_MAGIC && (r[1] ^ 0x5A5A5A5Au) == r[2] && r[1] < 1000000u)
		g.hi = g.kept_hi = r[1];
}

/* Music on or off, as last chosen */
static void settings_load(void)
{
	uint32_t r[2];

	backup_read(SETTINGS_AT, r, sizeof(r));
	if (r[0] == SETTINGS_MAGIC)
		music_enable(r[1] != 0);
}

static void settings_keep(void)
{
	const uint32_t r[2] = { SETTINGS_MAGIC, (uint32_t)music_enabled() };
	backup_write(SETTINGS_AT, r, sizeof(r));
}

static void record_keep(void)
{
	if (g.hi <= g.kept_hi)
		return;
	const uint32_t r[3] = { RECORD_MAGIC, g.hi, g.hi ^ 0x5A5A5A5Au };
	backup_write(RECORD_AT, r, sizeof(r));
	g.kept_hi = g.hi;
}

/* ---- the waves ------------------------------------------------------------------ */

static int boss_wave(void)
{
	return g.wave % 5 == 0;
}

/* The shape of the formation, four in turn */
static int in_shape(unsigned shape, int r, int c)
{
	switch (shape % 4) {
	case 1:	/* a wedge, its point up */
		return (c * 2 - (COLS - 1) <= r * 2 + 1) && ((COLS - 1) - c * 2 <= r * 2 + 1);
	case 2:	/* two blocks */
		return c < 4 || c > 5;
	case 3:	/* a checkerboard, the top row whole */
		return r == 0 || ((r + c) & 1) == 0;
	default:
		return 1;
	}
}

static const char *shape_name(unsigned shape)
{
	static const char *const names[4] = { "THE WALL", "THE WEDGE", "THE PINCERS", "THE CHECKERS" };
	return names[shape % 4];
}

/* ---- the formation -------------------------------------------------------- */

static const sprite_t *alien_sprite(int row, int armoured, int hp, unsigned pose)
{
	if (armoured)
		return hp < 2 ? &spr_tank_cracked : pose ? &spr_tank_1 : &spr_tank_0;
	if (row == 0)
		return pose ? &spr_watcher_1 : &spr_watcher_0;
	if (row < 3)
		return pose ? &spr_jelly_1 : &spr_jelly_0;
	return pose ? &spr_beetle_1 : &spr_beetle_0;
}

static unsigned alien_points(int row, int armoured)
{
	return armoured ? 40 : row == 0 ? 30 : row < 3 ? 20 : 10;
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

/* A box going over the shields wipes out what it covers */
static void shields_trample(int x, int y, int w, int h)
{
	if (y + h < SHIELD_Y || y > SHIELD_Y + SHIELD_H * Z)
		return;
	for (int s = 0; s < SHIELDS; s++)
		for (int cy = 0; cy < SHIELD_H; cy++)
			for (int cx = 0; cx < SHIELD_W; cx++) {
				const int px = shield_x(s) + cx * Z, py = SHIELD_Y + cy * Z;
				if (px + Z > x && px < x + w && py + Z > y && py < y + h)
					g.shield[s][cy][cx] = 0;
			}
}

/* ---- the game ------------------------------------------------------------------ */

static void clear_flying(void)
{
	for (int i = 0; i < SHOTS; i++)
		g.shot_on[i] = 0;
	g.fire_kept = 0;
	for (int b = 0; b < BOMBS; b++)
		g.bomb_on[b] = 0;
}

static void wave_start(void)
{
	const unsigned shape = (g.wave - 1) - (g.wave - 1) / 5;	/* the shapes, the boss waves left out */

	g.count = 0;
	for (int r = 0; r < ROWS; r++)
		for (int c = 0; c < COLS; c++) {
			/* armoured rows from the top: one from wave 3, two from 7, three from 11 */
			const int armoured = (g.wave >= 3 && r == 0) || (g.wave >= 7 && r == 1) || (g.wave >= 11 && r == 2);
			const int there = !boss_wave() && in_shape(shape, r, c);
			g.hp[r][c] = (uint8_t)(there ? (armoured ? 2 : 1) : 0);
			g.armoured[r][c] = (uint8_t)armoured;
			g.count += there;
		}
	g.left = (SCREEN_W - COLS * CELL_W) / 2;
	/* each wave a little lower, up to five */
	g.top = TOP + 40 + (int)((g.wave - 1) % 6) * 14;
	g.dir = 1;
	g.step_wait = step_wait_of(g.count);
	g.pose = 0;
	g.dive_wait = 240;
	for (int d = 0; d < DIVERS; d++)
		g.divers[d].on = 0;
	clear_flying();
	g.raider_on = 0;
	sound_stop(V_SIREN);
	g.raider_wait = 900 + rnd(600);
	shields_build();
	g.banner = 150;

	g.boss_on = boss_wave();
	music_play(g.boss_on ? MUSIC_BOSS : MUSIC_GAME);
	if (g.boss_on) {
		g.boss_max = g.boss_hp = 30 + 10 * (int)(g.wave / 5 - 1);
		g.boss_x = (SCREEN_W - BOSS_W) / 2;
		g.boss_y = TOP + 50;
		g.boss_dir = 1;
		g.boss_fire = 90;
		g.boss_flash = 0;
		g.count = 1;
		sound_play(V_JINGLE, SND_WARNING, 1024, 13, 0);
	}
}

static void game_start(void)
{
	g.score = 0;
	g.lives = 3;
	g.wave = START_WAVE;
	g.x = (SCREEN_W - FIGHTER_W) / 2;
	wave_start();
	g.state = PLAY;
	sound_play(V_JINGLE, SND_START, 1024, 13, 0);
}

static void explode(const sprite_t *what, int x, int y, int z)
{
	g.boom = what;
	g.boom_x = x;
	g.boom_y = y;
	g.boom_z = z;
	g.boom_time = 16;
}

static void show_bonus(unsigned points, int x, int y)
{
	g.bonus = points;
	g.bonus_x = x;
	g.bonus_y = y;
	g.bonus_time = 90;
}

static void lose_planet(void)
{
	g.lives = 1;	/* the last one, lost now */
	explode(&spr_blast, g.x - Z, FIGHTER_Y, Z);
	sound_play(V_BLAST, SND_BLAST, 1024, 13, 0);
	g.state = DYING;
	g.timer = 120;
}

static void formation_step(void)
{
	/* the columns still standing at each side */
	int first = COLS, last = -1, bottom = -1;
	for (int c = 0; c < COLS; c++)
		for (int r = 0; r < ROWS; r++)
			if (g.hp[r][c]) {
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
	/* four notes going down, one a step */
	static const unsigned notes[4] = { 1024, 912, 812, 768 };
	sound_play(V_STEP, SND_STEP, notes[g.steps++ & 3], 12, 0);

	for (int r = 0; r < ROWS; r++)
		for (int c = 0; c < COLS; c++)
			if (g.hp[r][c])
				shields_trample(alien_x(c), alien_y(r), ALIEN_W, ALIEN_H);
	/* down to the fighter: the planet is lost */
	if (alien_y(bottom) + ALIEN_H >= FIGHTER_Y)
		lose_planet();
}

static int free_bomb(void)
{
	for (int b = 0; b < BOMBS; b++)
		if (!g.bomb_on[b])
			return b;
	return -1;
}

static void drop_bomb(int x, int y, int dx)
{
	const int b = free_bomb();
	if (b < 0)
		return;
	g.bomb_on[b] = 1;
	g.bomb_x[b] = x;
	g.bomb_y[b] = y;
	g.bomb_dx[b] = dx;
}

static void bombs_drop(void)
{
	/* more bombs as the waves go */
	const unsigned chance = g.wave < 5 ? 40 - g.wave * 5 : 15;
	if (rnd(chance))
		return;
	/* the lowest alien of a column, the one over the fighter more often */
	int c = (int)rnd(COLS);
	if (rnd(3) == 0) {
		const int over = (g.x + FIGHTER_W / 2 - g.left) / CELL_W;
		if (over >= 0 && over < COLS)
			c = over;
	}
	for (int r = ROWS - 1; r >= 0; r--)
		if (g.hp[r][c]) {
			drop_bomb(alien_x(c) + ALIEN_W / 2 - 1, alien_y(r) + ALIEN_H, 0);
			return;
		}
}

static void fighter_hit(void)
{
	explode(&spr_blast, g.x - Z, FIGHTER_Y, Z);
	sound_play(V_BLAST, SND_BLAST, 1024, 13, pan_of(g.x));
	g.state = DYING;
	g.timer = 100;
	clear_flying();
}

static int touches_fighter(int x, int y, int w, int h)
{
	return x + w > g.x + 2 && x < g.x + FIGHTER_W - 2 && y + h > FIGHTER_Y + 2 && y < FIGHTER_Y + 8 * Z;
}

/* ---- the divers --------------------------------------------------------------- */

static void divers_launch(void)
{
	if (g.wave < 2 || boss_wave())
		return;
	if (g.dive_wait) {
		g.dive_wait--;
		return;
	}
	/* sooner as the waves go */
	const unsigned base = g.wave < 10 ? 300 - g.wave * 20 : 100;
	g.dive_wait = base + rnd(base);
	int d = 0;
	while (d < DIVERS && g.divers[d].on)
		d++;
	/* no more at once than the wave allows */
	if (d == DIVERS || d >= (int)(1 + g.wave / 4))
		return;
	const int c = (int)rnd(COLS);
	for (int r = ROWS - 1; r >= 0; r--)
		if (g.hp[r][c]) {
			diver_t *const v = &g.divers[d];
			v->on = 1;
			v->r = r;
			v->c = c;
			v->x = alien_x(c);
			v->y = alien_y(r);
			v->t = 0;
			v->back = 0;
			v->hp = g.hp[r][c];
			v->armoured = g.armoured[r][c];
			g.hp[r][c] = 0;
			sound_play(V_SWOOP, SND_SWOOP, 1024, 11, pan_of(v->x));
			return;
		}
}

static void divers_move(void)
{
	for (int d = 0; d < DIVERS; d++) {
		diver_t *const v = &g.divers[d];
		if (!v->on)
			continue;
		v->t++;
		if (!v->back) {
			/* down, weaving toward the fighter */
			v->y += 3 + (int)(g.wave > 6);
			const int aim = g.x + FIGHTER_W / 2 - ALIEN_W / 2;
			const int weave = ((v->t >> 4) & 1) ? 3 : -3;
			v->x += (aim > v->x ? 2 : -2) + weave;
			if (v->x < 0) v->x = 0;
			if (v->x > SCREEN_W - ALIEN_W) v->x = SCREEN_W - ALIEN_W;
			if (v->t % 40 == 20 && v->y < SHIELD_Y)
				drop_bomb(v->x + ALIEN_W / 2, v->y + ALIEN_H, 0);
			shields_trample(v->x, v->y, ALIEN_W, ALIEN_H);
			if (touches_fighter(v->x, v->y, ALIEN_W, ALIEN_H)) {
				v->on = 0;
				g.count--;
				g.step_wait = step_wait_of(g.count);
				fighter_hit();
				return;
			}
			if (v->y > GROUND) {
				v->back = 1;
				v->y = TOP - ALIEN_H;
			}
		} else {
			/* back from the top to its place in the formation */
			const int tx = alien_x(v->c), ty = alien_y(v->r);
			v->x += tx > v->x + 3 ? 3 : tx < v->x - 3 ? -3 : tx - v->x;
			v->y += ty > v->y + 3 ? 3 : ty < v->y - 3 ? -3 : ty - v->y;
			if (v->x == tx && v->y == ty) {
				g.hp[v->r][v->c] = (uint8_t)v->hp;
				v->on = 0;
			}
		}
	}
}

/* ---- the mothership ------------------------------------------------------------ */

static void boss_move(void)
{
	if (!g.boss_on)
		return;
	/* faster as it is hurt */
	const int speed = 2 + (g.boss_max - g.boss_hp) * 3 / g.boss_max;
	g.boss_x += g.boss_dir * speed;
	if (g.boss_x < 8 || g.boss_x > SCREEN_W - 8 - BOSS_W)
		g.boss_dir = -g.boss_dir;
	g.boss_y = TOP + 50 + (int)((g.frame >> 2) % 40 < 20 ? (g.frame >> 2) % 20 : 20 - (g.frame >> 2) % 20);
	if (g.boss_flash)
		g.boss_flash--;
	if (g.boss_fire) {
		g.boss_fire--;
	} else {
		/* a fan of three, five when it is half gone */
		const int cx = g.boss_x + BOSS_W / 2, by = g.boss_y + BOSS_H;
		drop_bomb(cx, by, 0);
		drop_bomb(cx - 12, by, -1);
		drop_bomb(cx + 12, by, 1);
		if (g.boss_hp * 2 < g.boss_max) {
			drop_bomb(cx - 30, by - 6, -2);
			drop_bomb(cx + 30, by - 6, 2);
		}
		g.boss_fire = 70 - (unsigned)((g.boss_max - g.boss_hp) * 30 / g.boss_max);
	}
}

/* Whether a shot at sx, sy hits the mothership; its end if it is the last hit */
static int boss_hit(int sx, int sy)
{
	if (!g.boss_on || sx + 2 <= g.boss_x || sx >= g.boss_x + BOSS_W || sy >= g.boss_y + BOSS_H ||
	    sy + 10 + SHOT_SPEED <= g.boss_y)
		return 0;
	g.boss_flash = 4;
	if (--g.boss_hp > 0) {
		g.score += 10;
		sound_play(V_CLANG, SND_CLANG, 900 + rnd(250), 12, pan_of(sx));
		return 1;
	}
	const unsigned points = 1000 * (g.wave / 5);
	g.score += points;
	show_bonus(points, g.boss_x + BOSS_W / 2 - 24, g.boss_y);
	explode(&spr_blast, g.boss_x + BOSS_W / 2 - 24, g.boss_y, 3);
	sound_play(V_BLAST, SND_BLAST, 800, 15, 0);
	g.boss_on = 0;
	g.count = 0;
	return 1;
}

/* ---- a frame of play ------------------------------------------------------------ */

static void alien_down(int x, int y, unsigned points)
{
	g.count--;
	g.score += points;
	g.step_wait = step_wait_of(g.count);
	explode(&spr_boom, x, y, Z);
	sound_play(V_BOOM, SND_BOOM, 960 + rnd(160), 13, pan_of(x));
}

/* A shot that hit an alien there: hit then */
static int shot_hits_formation(int sx, int sy)
{
	for (int r = 0; r < ROWS; r++)
		for (int c = 0; c < COLS; c++) {
			if (!g.hp[r][c])
				continue;
			const int ax = alien_x(c), ay = alien_y(r);
			/* from where the shot was to where it is: none slips through */
			if (sx + 2 > ax && sx < ax + ALIEN_W && sy < ay + ALIEN_H && sy + 10 + SHOT_SPEED > ay) {
				if (--g.hp[r][c] == 0)
					alien_down(ax, ay, alien_points(r, g.armoured[r][c]));
				else
					sound_play(V_CLANG, SND_CLANG, 1300, 10, pan_of(ax));
				return 1;
			}
		}
	for (int d = 0; d < DIVERS; d++) {
		diver_t *const v = &g.divers[d];
		if (v->on && sx + 2 > v->x && sx < v->x + ALIEN_W && sy < v->y + ALIEN_H && sy + 10 + SHOT_SPEED > v->y) {
			if (--v->hp == 0) {
				v->on = 0;
				/* twice the points out of the formation */
				const unsigned points = 2 * alien_points(v->r, v->armoured);
				alien_down(v->x, v->y, points);
				show_bonus(points, v->x, v->y);
			} else {
				sound_play(V_CLANG, SND_CLANG, 1300, 10, pan_of(v->x));
			}
			return 1;
		}
	}
	return 0;
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
	if (g.x > SCREEN_W - 8 - FIGHTER_W)
		g.x = SCREEN_W - 8 - FIGHTER_W;
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
				sound_play(V_SHOT, SND_SHOOT, 1024, 11, pan_of(g.x));
				break;
			}
	}

	/* the shots */
	for (int i = 0; i < SHOTS; i++) {
		if (!g.shot_on[i])
			continue;
		g.shot_y[i] -= SHOT_SPEED;
		const int sx = g.shot_x[i], sy = g.shot_y[i];
		if (sy < TOP || shield_hit(sx, sy) || shield_hit(sx, sy + 8) || shot_hits_formation(sx, sy) ||
		    boss_hit(sx, sy)) {
			g.shot_on[i] = 0;
			continue;
		}
		if (g.raider_on && sy < TOP + 7 * Z + 4 && sx >= g.raider_x && sx < g.raider_x + 16 * Z) {
			static const unsigned bonuses[4] = { 50, 100, 150, 300 };
			const unsigned points = bonuses[rnd(4)];
			g.score += points;
			show_bonus(points, g.raider_x, TOP + 6);
			g.raider_on = 0;
			g.shot_on[i] = 0;
			sound_stop(V_SIREN);
			sound_play(V_JINGLE, SND_BONUS, 1024, 13, pan_of(g.raider_x));
			explode(&spr_blast, g.raider_x, TOP + 4, Z);
		}
	}

	/* the formation, its divers, the mothership */
	if (g.timer == 0) {
		formation_step();
		g.timer = g.step_wait;
	} else {
		g.timer--;
	}
	if (g.state != PLAY)
		return;
	divers_launch();
	divers_move();
	if (g.state != PLAY)
		return;
	boss_move();
	if (g.count == 0) {
		g.state = CLEARED;
		g.timer = 150;
		sound_stop(V_SIREN);
		g.raider_on = 0;
		music_stop();
		sound_play(V_JINGLE, SND_CLEARED, 1024, 13, 0);
		return;
	}

	/* the bombs */
	if (!g.boss_on)
		bombs_drop();
	for (int b = 0; b < BOMBS; b++) {
		if (!g.bomb_on[b])
			continue;
		g.bomb_y[b] += 4 + (g.wave > 3);
		if (g.frame & 1)
			g.bomb_x[b] += g.bomb_dx[b];
		if (g.bomb_y[b] > GROUND || g.bomb_x[b] < 0 || g.bomb_x[b] > SCREEN_W ||
		    shield_hit(g.bomb_x[b], g.bomb_y[b] + 10)) {
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
		if (touches_fighter(g.bomb_x[b], g.bomb_y[b], 3, 10)) {
			fighter_hit();
			return;
		}
	}

	/* the raider, not over the mothership */
	if (g.raider_on) {
		g.raider_x += g.raider_dir * 2;
		if (g.raider_x < -16 * Z || g.raider_x > SCREEN_W) {
			g.raider_on = 0;
			sound_stop(V_SIREN);
		}
	} else if (g.raider_wait == 0) {
		if (!g.boss_on) {
			g.raider_on = 1;
			g.raider_dir = rnd(2) ? 1 : -1;
			g.raider_x = g.raider_dir > 0 ? -16 * Z : SCREEN_W;
			sound_play(V_SIREN, SND_SIREN, 1024, 9, 0);
		}
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
	draw_number(12 + text_width("SCORE ", 2), 10, g.score, 6, WHITE, 2);
	draw_text(SCREEN_W / 2 - text_width("HI 000000", 2) / 2 + 20, 10, "HI", GREY, 2);
	draw_number(SCREEN_W / 2 - text_width("HI 000000", 2) / 2 + 20 + text_width("HI ", 2), 10, g.hi, 6, YELLOW, 2);
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
	/* the formation, its divers */
	for (int r = 0; r < ROWS; r++)
		for (int c = 0; c < COLS; c++)
			if (g.hp[r][c])
				draw_sprite_scaled(alien_sprite(r, g.armoured[r][c], g.hp[r][c], g.pose), alien_x(c), alien_y(r), Z);
	for (int d = 0; d < DIVERS; d++)
		if (g.divers[d].on)
			draw_sprite_scaled(alien_sprite(g.divers[d].r, g.divers[d].armoured, g.divers[d].hp, (g.frame >> 3) & 1),
			                   g.divers[d].x, g.divers[d].y, Z);
	/* the mothership and its strength */
	if (g.boss_on) {
		if (!(g.boss_flash & 2))
			draw_sprite_scaled(&spr_mothership, g.boss_x, g.boss_y, BOSS_Z);
		const int bar = 300 * g.boss_hp / g.boss_max;
		fill_rect((SCREEN_W - 300) / 2, TOP - 2, 300, 6, rgb(60, 20, 20));
		fill_rect((SCREEN_W - 300) / 2, TOP - 2, bar, 6, g.boss_hp * 4 < g.boss_max ? RED : ORANGE);
	}
	if (g.raider_on)
		draw_sprite_scaled(&spr_raider, g.raider_x, TOP + 4, Z);
	if (g.bonus_time) {
		g.bonus_time--;
		draw_number(g.bonus_x + 4, g.bonus_y, g.bonus, g.bonus >= 1000 ? 4 : 3, RED, 2);
	}
	if (g.boom_time) {
		g.boom_time--;
		draw_sprite_scaled(g.boom, g.boom_x, g.boom_y, g.boom_z);
	}
	/* the shots, the bombs */
	for (int i = 0; i < SHOTS; i++)
		if (g.shot_on[i])
			fill_rect(g.shot_x[i], g.shot_y[i], 2, 10, WHITE);
	for (int b = 0; b < BOMBS; b++)
		if (g.bomb_on[b]) {
			const int wiggle = ((g.bomb_y[b] >> 3) & 1) ? 2 : 0;
			const uint16_t colour = g.bomb_dx[b] ? ORANGE : YELLOW;
			fill_rect(g.bomb_x[b] + wiggle, g.bomb_y[b], 2, 4, colour);
			fill_rect(g.bomb_x[b] + 2 - wiggle, g.bomb_y[b] + 4, 2, 4, colour);
			fill_rect(g.bomb_x[b] + wiggle, g.bomb_y[b] + 8, 2, 3, colour);
		}
	/* the fighter, unless it is the one exploding */
	if (g.state != DYING && g.state != OVER)
		draw_sprite_scaled(&spr_fighter, g.x, FIGHTER_Y, Z);
	/* the ground, the fighters left */
	fill_rect(0, GROUND, SCREEN_W, 2, GREEN);
	draw_number(12, GROUND + 8, g.lives, 1, WHITE, 2);
	for (unsigned l = 1; l < g.lives && l < 6; l++)
		draw_sprite(&spr_fighter, 30 + (int)l * 22, GROUND + 10);
	/* the name of the wave as it comes */
	if (g.banner && g.state == PLAY) {
		g.banner--;
		if (boss_wave()) {
			if ((g.banner >> 3) & 1)
				draw_centred(SCREEN_H / 2 - 10, "WARNING: MOTHERSHIP", RED, 3);
		} else {
			const unsigned shape = (g.wave - 1) - (g.wave - 1) / 5;
			draw_centred(SCREEN_H / 2, shape_name(shape), GREY, 3);
		}
	}
}

static void draw_title(void)
{
	draw_centred(64, "NOVA SIEGE", YELLOW, 7);
	draw_centred(130, "HOLD THE LINE", GREY, 2);

	const sprite_t *table[5] = { &spr_raider, &spr_tank_0, &spr_watcher_0, &spr_jelly_0, &spr_beetle_0 };
	const char *points[5] = { "= ? MYSTERY", "= 40  TWO HITS", "= 30 POINTS", "= 20 POINTS", "= 10 POINTS" };
	for (int i = 0; i < 5; i++) {
		const int y = 172 + i * 34;
		draw_sprite_scaled(table[i], 200 - table[i]->w * Z / 2, y, Z);
		draw_text(240, y + 2, points[i], WHITE, 2);
	}
	draw_centred(345, "DIVERS SCORE DOUBLE  -  MOTHERSHIP EVERY 5TH WAVE", ORANGE, 1);
	if (!input_ready())
		draw_centred(385, "WAITING FOR THE I/O BOARD...", RED, 2);
	else if ((g.frame >> 5) & 1)
		draw_centred(380, "PRESS START", WHITE, 3);
	draw_centred(418, music_enabled() ? "B / B2: MUSIC ON" : "B / B2: MUSIC OFF", GREY, 2);
	draw_centred(445, "STICK: MOVE   A / B1: FIRE", GREY, 2);
}

/* ---- the loop ---------------------------------------------------------------------- */

void main(void)
{
	uint16_t before = 0;

	layer_init();
	input_init();
	sound_init();
	record_load();
	settings_load();
#ifdef RECORD_TEST	/* for the tests: a record kept at power-on */
	g.hi = RECORD_TEST;
	record_keep();
#endif
	g.state = TITLE;
	g.wave = 1;
	g.lives = 3;
	wave_start();
	g.banner = 0;
	sound_stop(V_JINGLE);

	for (;;) {
		g.frame++;
		seed += g.frame;
		const uint16_t in = input_read();
		const uint16_t pressed = (uint16_t)(in & ~before);
		before = in;

		switch (g.state) {
		case TITLE:
			music_play(MUSIC_TITLE);
			if (pressed & (IN_B2 | IN_B3)) {
				music_enable(!music_enabled());
				settings_keep();
			}
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
					sound_stop(V_SIREN);
					g.raider_on = 0;
					music_stop();
					sound_play(V_JINGLE, SND_OVER, 1024, 13, 0);
					record_keep();
				} else {
					g.state = PLAY;
					g.x = (SCREEN_W - FIGHTER_W) / 2;
				}
			}
			break;
		case CLEARED:
			if (--g.timer == 0) {
				g.wave++;
				record_keep();
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
		sound_frame();
		music_frame();

		clear(SKY);
		draw_scores();
		if (g.state == TITLE) {
			draw_title();
		} else {
			draw_field();
			if (g.state == CLEARED)
				draw_centred(SCREEN_H / 2 - 20, "WAVE CLEARED", YELLOW, 4);
			if (g.state == OVER) {
				draw_centred(SCREEN_H / 2 - 40, "GAME OVER", RED, 6);
				if (g.score && g.score == g.hi)
					draw_centred(SCREEN_H / 2 + 30, "NEW RECORD!", YELLOW, 3);
			}
		}
		screen_flip();
	}
}
