/*
 * main.c: GLOWBUG GARDEN, for the SEGA Hikaru under Odin.
 *
 * A firefly roams the paths of a garden at night and lights every seed of
 * light. Three shadow beetles hunt it, each its own way: one heads for the
 * firefly, one for where it is going, one wanders and turns on it when
 * near. A lantern flower turns them for a while: they flee, and the
 * firefly sends them back to their burrow. Four gardens, then they come
 * round again, faster.
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

/* ---- the garden -------------------------------------------------------------- */

#define TILE   16
#define MW     28
#define MH     23
#define MAZE_X ((SCREEN_W - MW * TILE) / 2)
#define MAZE_Y 70
#define MAZES  4

/* '#' a hedge, '.' a seed, 'o' a lantern flower, 'B' the burrow, 'P' where
 * the firefly starts. Made for the game: symmetric, no dead end. */
static const char *const mazes[MAZES][MH] = {
	{ /* the lawn */
		"############################",
		"#o........................o#",
		"#.###.#####.#..#.#####.###.#",
		"#...#.......#..#.......#...#",
		"###.#.#######..#######.#.###",
		"#.....#...#......#...#.....#",
		"#.#.###.#.#.#..#.#.#.###.#.#",
		"#.#.....#..........#.....#.#",
		"#.#######.###BB###.#######.#",
		"#.........#BBBBBB#.........#",
		"#.#.#####.#BBBBBB#.#####.#.#",
		"#.#.....#.#BBBBBB#.#.....#.#",
		"#.###.#.#.########.#.#.###.#",
		"#.#...#..............#...#.#",
		"#.#.#.###.########.###.#.#.#",
		"#...#.......#..#.......#...#",
		"#.#######.#.#..#.#.#######.#",
		"#.........#..P ..#.........#",
		"###.#.#.#####..#####.#.#.###",
		"#...#.#.#..........#.#.#...#",
		"#.###.#.#.########.#.#.###.#",
		"#o....#..............#....o#",
		"############################",
	},
	{ /* the beds */
		"############################",
		"#o..........#..#..........o#",
		"#.###.#####.#..#.#####.###.#",
		"#.#...#.....#..#.....#...#.#",
		"#.#.#.#.#####..#####.#.#.#.#",
		"#.....#..............#.....#",
		"#.###.#.#####..#####.#.###.#",
		"#.....#.#..........#.#.....#",
		"#.#####.#.###BB###.#.#####.#",
		"#.........#BBBBBB#.........#",
		"###.#.#.#.#BBBBBB#.#.#.#.###",
		"#...#...#.#BBBBBB#.#...#...#",
		"#.#.#.#.#.########.#.#.#.#.#",
		"#.....#..............#.....#",
		"#.###.#####.####.#####.###.#",
		"#...#.....#.#..#.#.....#...#",
		"###.###.#.#.#..#.#.#.###.###",
		"#.....#.#...#P #...#.#.....#",
		"#.###.#.#.###..###.#.#.###.#",
		"#.......#.#......#.#.......#",
		"#.#.#.#.#.#.#..#.#.#.#.#.#.#",
		"#o..#.......#..#.......#..o#",
		"############################",
	},
	{ /* the rows */
		"############################",
		"#o..#.......#..#.......#..o#",
		"#.#.#.#.#.#.#..#.#.#.#.#.#.#",
		"#.#.#.#...#......#...#.#.#.#",
		"#.#.#.#.#.#.####.#.#.#.#.#.#",
		"#...#.#.#.#......#.#.#.#...#",
		"#.###.#.#.#.#..#.#.#.#.###.#",
		"#...#..................#...#",
		"###.#####.###BB###.#####.###",
		"#...#.....#BBBBBB#.....#...#",
		"#.#.#.###.#BBBBBB#.###.#.#.#",
		"#.#...#...#BBBBBB#...#...#.#",
		"#.#####.#.########.#.#####.#",
		"#..........................#",
		"#.#.#########..#########.#.#",
		"#.#......................#.#",
		"#.###.#.#####..#####.#.###.#",
		"#.#...#.#...#P #...#.#...#.#",
		"#.#.#.#.#.#.#..#.#.#.#.#.#.#",
		"#.#.....#.#......#.#.....#.#",
		"#.#.#.#.#.#.####.#.#.#.#.#.#",
		"#o..#..................#..o#",
		"############################",
	},
	{ /* the cloister */
		"############################",
		"#o....#...#......#...#....o#",
		"#.###.#.#.#.#..#.#.#.#.###.#",
		"#...#.#.#...#..#...#.#.#...#",
		"#.#.#.#.############.#.#.#.#",
		"#.#...#..............#...#.#",
		"#.#######.###..###.#######.#",
		"#.......#..........#.......#",
		"#.###.#.#.###BB###.#.#.###.#",
		"#.....#...#BBBBBB#...#.....#",
		"#.#######.#BBBBBB#.#######.#",
		"#.....#...#BBBBBB#...#.....#",
		"#####.#.#.########.#.#.#####",
		"#.....#..............#.....#",
		"#.#######.#.#..#.#.#######.#",
		"#...#...#.#.#..#.#.#...#...#",
		"###.#.#.#.#.#..#.#.#.#.#.###",
		"#...#.#...#..P ..#...#.#...#",
		"#.###.#######..#######.###.#",
		"#...#.#...#......#...#.#...#",
		"#.#.#.#.#.#.#..#.#.#.#.#.#.#",
		"#o......#...#..#...#......o#",
		"############################",
	},
};

#define BEETLES 3
#define EXIT_X  13		/* the tile above the burrow's door */
#define EXIT_Y  7
#define HOME_X  13		/* inside the burrow */
#define HOME_Y  10

/* Voices of the AICA */
enum { V_SEED, V_FLOWER, V_BEETLE, V_CAUGHT, V_JINGLE };

#define RECORD_AT 0xF000u
#define RECORD_MAGIC 0x574F4C47u	/* "GLOW" */
#define SETTINGS_AT (RECORD_AT + 16u)
#define SETTINGS_MAGIC 0x53574C47u	/* "GLWS" */

#define WHITE  rgb(240, 240, 250)
#define GREY   rgb(150, 150, 180)
#define YELLOW rgb(250, 220, 80)
#define RED    rgb(250, 80, 60)
#define GLOW   rgb(210, 255, 110)

enum { RIGHT, DOWN, LEFT, UP };
static const int dx[4] = { 1, 0, -1, 0 }, dy[4] = { 0, 1, 0, -1 };

typedef enum { TITLE, READY, PLAY, CAUGHT, CLEARED, OVER } state_t;
typedef enum { WAITING, LEAVING, HUNTING, FLEEING, RETURNING } mood_t;

typedef struct {
	int x, y;		/* pixels from the maze's corner; on a tile when both are multiples of TILE */
	int dir, acc;
} mover_t;

typedef struct {
	mover_t m;
	mood_t mood;
	unsigned wait;
} beetle_t;

static struct {
	state_t state;
	unsigned timer, frame;
	unsigned score, hi, kept_hi, lives, level, round, next_life;

	char maze[MH][MW + 1];
	unsigned seeds;			/* still to light */

	mover_t fly;
	int want;
	beetle_t beetles[BEETLES];
	unsigned fright;		/* frames the beetles still flee */
	unsigned chain;			/* beetles sent home on this flower */

	int pop_x, pop_y;
	unsigned pop_points, pop_time;
	unsigned seed_note;
} g;

static unsigned seed = 0x1F2E3D4Cu;

static unsigned rnd(unsigned n)
{
	seed = seed * 1103515245u + 12345u;
	return (seed >> 16) % n;
}

static int pan_of(int x)
{
	const int p = (x - MW * TILE / 2) / 32;
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

/* ---- the tiles ------------------------------------------------------------------ */

static char tile(int tx, int ty)
{
	if (tx < 0 || tx >= MW || ty < 0 || ty >= MH)
		return '#';
	return g.maze[ty][tx];
}

static int open_for_fly(int tx, int ty)
{
	const char t = tile(tx, ty);
	return t != '#' && t != 'B';
}

static int open_for_beetle(const beetle_t *b, int tx, int ty)
{
	const char t = tile(tx, ty);
	if (t == '#')
		return 0;
	/* the burrow is theirs only to leave it, or to go home */
	return t != 'B' || b->mood == LEAVING || b->mood == RETURNING || b->mood == WAITING;
}

static int on_tile(const mover_t *m)
{
	return m->x % TILE == 0 && m->y % TILE == 0;
}

/* ---- a level ---------------------------------------------------------------------- */

static void place_movers(void)
{
	for (int y = 0; y < MH; y++)
		for (int x = 0; x < MW; x++)
			if (g.maze[y][x] == 'P') {
				g.fly.x = x * TILE + TILE / 2;	/* between two tiles of the wide middle path */
				g.fly.y = y * TILE;
			}
	g.fly.x -= TILE / 2;
	g.fly.dir = LEFT;
	g.fly.acc = 0;
	g.want = -1;

	static const int bx[BEETLES] = { 12, 14, 15 };
	const unsigned faster = g.level - 1 + g.round * 4;
	for (int i = 0; i < BEETLES; i++) {
		beetle_t *const b = &g.beetles[i];
		b->m.x = bx[i] * TILE;
		b->m.y = HOME_Y * TILE;
		b->m.dir = UP;
		b->m.acc = 0;
		b->mood = WAITING;
		const unsigned wait = (unsigned)i * 240u;
		b->wait = wait > faster * 30u ? wait - faster * 30u : 0;
	}
	g.fright = 0;
}

static void level_start(void)
{
	const char *const *map = mazes[(g.level - 1) % MAZES];

	g.seeds = 0;
	for (int y = 0; y < MH; y++) {
		for (int x = 0; x < MW; x++) {
			g.maze[y][x] = map[y][x];
			if (map[y][x] == '.' || map[y][x] == 'o')
				g.seeds++;
		}
		g.maze[y][MW] = 0;
	}
	place_movers();
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
	g.next_life = 10000;
	sound_play(V_JINGLE, SND_START, 1024, 13, 0);
	level_start();
}

static void score(unsigned points)
{
	g.score += points;
	if (g.score >= g.next_life) {
		g.next_life += 15000;
		g.lives++;
		sound_play(V_JINGLE, SND_LIFE, 1024, 13, 0);
	}
}

/* ---- the speeds, in 16ths of a pixel a frame --------------------------------------- */

static int fly_speed(void)
{
	const int s = 22 + (int)(g.level - 1) + (int)g.round * 3;
	return s > 30 ? 30 : s;
}

static int beetle_speed(const beetle_t *b)
{
	if (b->mood == RETURNING)
		return 48;
	if (b->mood == FLEEING)
		return 12;
	if (b->mood == WAITING || b->mood == LEAVING)
		return 12;
	const int s = 19 + (int)(g.level - 1) + (int)g.round * 3;
	return s > 29 ? 29 : s;
}

/* ---- the firefly ---------------------------------------------------------------------- */

static void eat(int tx, int ty)
{
	const char t = tile(tx, ty);

	if (t == '.') {
		g.maze[ty][tx] = ' ';
		g.seeds--;
		score(10);
		g.seed_note ^= 1;
		sound_play(V_SEED, SND_SEED, g.seed_note ? 1024 : 1180, 9, pan_of(tx * TILE));
	} else if (t == 'o') {
		g.maze[ty][tx] = ' ';
		g.seeds--;
		score(50);
		sound_play(V_FLOWER, SND_FLOWER, 1024, 12, pan_of(tx * TILE));
		const int f = 420 - (int)(g.level - 1) * 30 - (int)g.round * 90;
		g.fright = f < 120 ? 120 : (unsigned)f;
		g.chain = 0;
		for (int i = 0; i < BEETLES; i++) {
			beetle_t *const b = &g.beetles[i];
			if (b->mood == HUNTING || b->mood == FLEEING) {
				if (b->mood == HUNTING)
					b->m.dir = (b->m.dir + 2) % 4;
				b->mood = FLEEING;
			}
		}
	}
	if (g.seeds == 0) {
		g.state = CLEARED;
		g.timer = 180;
		music_stop();
		sound_play(V_JINGLE, SND_CLEARED, 1024, 13, 0);
		score(1000);
	}
}

static void fly_move(void)
{
	mover_t *const m = &g.fly;

	/* back the way it came, at once */
	if (g.want == (m->dir + 2) % 4)
		m->dir = g.want;
	m->acc += fly_speed();
	for (int n = m->acc >> 4; n > 0 && g.state == PLAY; n--) {
		if (on_tile(m)) {
			const int tx = m->x / TILE, ty = m->y / TILE;
			eat(tx, ty);
			if (g.state != PLAY)
				break;
			if (g.want >= 0 && open_for_fly(tx + dx[g.want], ty + dy[g.want]))
				m->dir = g.want;
			if (!open_for_fly(tx + dx[m->dir], ty + dy[m->dir]))
				break;
		}
		m->x += dx[m->dir];
		m->y += dy[m->dir];
	}
	m->acc &= 15;
}

/* ---- the beetles --------------------------------------------------------------------- */

static void target_of(int i, const beetle_t *b, int *tx, int *ty)
{
	const int fx = (g.fly.x + TILE / 2) / TILE, fy = (g.fly.y + TILE / 2) / TILE;

	if (b->mood == LEAVING) {
		*tx = EXIT_X;
		*ty = EXIT_Y;
	} else if (b->mood == RETURNING) {
		*tx = HOME_X;
		*ty = HOME_Y;
	} else if (i == 0) {		/* Ember: the firefly itself */
		*tx = fx;
		*ty = fy;
	} else if (i == 1) {		/* Moss: four tiles ahead of it */
		*tx = fx + 4 * dx[g.fly.dir];
		*ty = fy + 4 * dy[g.fly.dir];
	} else {			/* Dusk: a random corner, the firefly when near */
		const int bx = b->m.x / TILE, by = b->m.y / TILE;
		if ((bx - fx) * (bx - fx) + (by - fy) * (by - fy) < 64) {
			*tx = fx;
			*ty = fy;
		} else {
			*tx = (g.frame / 300) & 1 ? MW : -1;
			*ty = (g.frame / 600) & 1 ? MH : -1;
		}
	}
}

static void beetle_turn(int i, beetle_t *b)
{
	const int bx = b->m.x / TILE, by = b->m.y / TILE;
	const int back = (b->m.dir + 2) % 4;
	int options[4], n = 0;

	for (int d = 0; d < 4; d++)
		if (d != back && open_for_beetle(b, bx + dx[d], by + dy[d]))
			options[n++] = d;
	if (n == 0) {
		b->m.dir = back;
		return;
	}
	if (b->mood == FLEEING) {
		b->m.dir = options[rnd((unsigned)n)];
		return;
	}
	int tx, ty;
	target_of(i, b, &tx, &ty);
	int best = options[0], best_d = 1 << 30;
	for (int k = 0; k < n; k++) {
		const int d = options[k];
		const int ex = bx + dx[d] - tx, ey = by + dy[d] - ty;
		const int dist = ex * ex + ey * ey;
		if (dist < best_d) {
			best_d = dist;
			best = d;
		}
	}
	b->m.dir = best;
}

static void beetle_move(int i, beetle_t *b)
{
	if (b->mood == WAITING) {
		if (b->wait)
			b->wait--;
		else
			b->mood = LEAVING;
		return;
	}
	b->m.acc += beetle_speed(b);
	for (int n = b->m.acc >> 4; n > 0; n--) {
		if (on_tile(&b->m)) {
			const int bx = b->m.x / TILE, by = b->m.y / TILE;
			if (b->mood == LEAVING && bx == EXIT_X && by == EXIT_Y)
				b->mood = g.fright ? FLEEING : HUNTING;
			if (b->mood == RETURNING && bx == HOME_X && by == HOME_Y) {
				b->mood = LEAVING;
				sound_play(V_BEETLE, SND_HOME, 1024, 6, 0);
			}
			beetle_turn(i, b);
		}
		b->m.x += dx[b->m.dir];
		b->m.y += dy[b->m.dir];
	}
	b->m.acc &= 15;
}

static void touch(void)
{
	for (int i = 0; i < BEETLES; i++) {
		beetle_t *const b = &g.beetles[i];
		const int ex = b->m.x - g.fly.x, ey = b->m.y - g.fly.y;

		if ((ex < 0 ? -ex : ex) + (ey < 0 ? -ey : ey) >= 12)
			continue;
		if (b->mood == FLEEING) {
			const unsigned points = 200u << (g.chain < 3 ? g.chain : 3);
			g.chain++;
			score(points);
			b->mood = RETURNING;
			g.pop_x = b->m.x;
			g.pop_y = b->m.y;
			g.pop_points = points;
			g.pop_time = 60;
			sound_play(V_BEETLE, SND_BEETLE, 1024, 13, pan_of(b->m.x));
		} else if (b->mood == HUNTING) {
			g.state = CAUGHT;
			g.timer = 150;
			music_stop();
			sound_play(V_CAUGHT, SND_CAUGHT, 1024, 13, pan_of(g.fly.x));
			return;
		}
	}
}

static void play(void)
{
	fly_move();
	if (g.state != PLAY)
		return;
	if (g.fright && --g.fright == 0)
		for (int i = 0; i < BEETLES; i++)
			if (g.beetles[i].mood == FLEEING)
				g.beetles[i].mood = HUNTING;
	for (int i = 0; i < BEETLES; i++)
		beetle_move(i, &g.beetles[i]);
	touch();
	if (g.pop_time)
		g.pop_time--;
}

/* ---- the drawing ----------------------------------------------------------------------- */

static void draw_centred(int y, const char *text, uint16_t colour, int scale)
{
	draw_text((SCREEN_W - text_width(text, scale)) / 2, y, text, colour, scale);
}

static void draw_garden(void)
{
	const uint16_t leaf = rgb(30, 90, 45), light = rgb(60, 140, 70), dark = rgb(15, 50, 25);
	const uint16_t earth = rgb(45, 30, 22);

	for (int ty = 0; ty < MH; ty++)
		for (int tx = 0; tx < MW; tx++) {
			const int x = MAZE_X + tx * TILE, y = MAZE_Y + ty * TILE;
			const char t = g.maze[ty][tx];

			if (t == '#') {
				fill_rect(x, y, TILE, TILE, leaf);
				/* lit where a path is beside it, shaded below */
				if (tile(tx, ty - 1) != '#')
					fill_rect(x, y, TILE, 3, light);
				if (tile(tx - 1, ty) != '#')
					fill_rect(x, y, 2, TILE, light);
				if (tile(tx, ty + 1) != '#')
					fill_rect(x, y + TILE - 3, TILE, 3, dark);
				if (tile(tx + 1, ty) != '#')
					fill_rect(x + TILE - 2, y, 2, TILE, dark);
				if (((tx * 7 + ty * 13) % 5) == 0)
					fill_rect(x + 6, y + 6, 3, 3, light);
			} else if (t == 'B') {
				fill_rect(x, y, TILE, TILE, earth);
			} else if (t == '.') {
				const int twinkle = ((g.frame / 8 + (unsigned)(tx * 3 + ty * 5)) % 12) == 0;
				fill_rect(x + 6, y + 6, 4, 4, twinkle ? WHITE : rgb(170, 220, 90));
				fill_rect(x + 7, y + 7, 2, 2, rgb(250, 255, 200));
			} else if (t == 'o') {
				if ((g.frame / 12) & 1)
					fill_rect(x - 2, y - 2, TILE + 4, TILE + 4, rgb(60, 30, 60));
				draw_sprite(&spr_flower, x, y);
			}
		}
}

static void draw_beetles(void)
{
	static const sprite_t *const looks[BEETLES][2] = {
		{ &spr_ember_0, &spr_ember_1 }, { &spr_moss_0, &spr_moss_1 }, { &spr_dusk_0, &spr_dusk_1 },
	};
	const int step = (g.frame / 6) & 1;

	for (int i = 0; i < BEETLES; i++) {
		const beetle_t *const b = &g.beetles[i];
		const int x = MAZE_X + b->m.x, y = MAZE_Y + b->m.y;

		if (b->mood == RETURNING) {
			/* a shadow scuttling home: its eyes */
			fill_rect(x + 5, y + 3, 2, 2, WHITE);
			fill_rect(x + 9, y + 3, 2, 2, WHITE);
			continue;
		}
		if (b->mood == FLEEING) {
			const int pale = g.fright < 120 && ((g.frame / 8) & 1);
			draw_sprite(pale ? (step ? &spr_pale_1 : &spr_pale_0) : (step ? &spr_scared_1 : &spr_scared_0), x, y);
		} else {
			draw_sprite(looks[i][step], x, y);
		}
	}
}

static void draw_fly(void)
{
	const int x = MAZE_X + g.fly.x, y = MAZE_Y + g.fly.y;

	if (g.state == CAUGHT && g.timer < 100) {
		/* its light going out */
		if ((g.frame & 4) && g.timer > 30)
			draw_sprite(&spr_firefly_1, x, y);
		return;
	}
	/* a halo that breathes */
	const int r = 2 + (int)((g.frame / 10) % 3);
	fill_rect(x + 4 - r, y + 9 - r, 8 + 2 * r, 6 + 2 * r, rgb(40, 60, 20));
	draw_sprite((g.frame / 4) & 1 ? &spr_firefly_1 : &spr_firefly_0, x, y);
}

static void draw_scores(void)
{
	draw_text(12, 14, "SCORE", GREY, 2);
	draw_number(12, 34, g.score, 7, WHITE, 2);
	draw_text(SCREEN_W / 2 - 50, 14, "HI", GREY, 2);
	draw_number(SCREEN_W / 2 - 50, 34, g.hi, 7, YELLOW, 2);
	draw_text(SCREEN_W - 12 - text_width("LEVEL 00", 2), 14, "LEVEL", GREY, 2);
	draw_number(SCREEN_W - 12 - text_width("00", 2), 14, g.level, 2, WHITE, 2);
	if (g.state != TITLE)
		for (unsigned i = 1; i < g.lives && i < 8; i++)
			draw_sprite(&spr_firefly_0, SCREEN_W - 12 - (int)i * 20, 34);
}

static void draw_title(void)
{
	clear(rgb(6, 10, 18));
	/* fireflies drifting */
	for (int i = 0; i < 14; i++) {
		const int x = (int)((i * 97 + g.frame / 2 * (1 + i % 3)) % SCREEN_W);
		const int y = 90 + (i * 53) % 330 + (int)((g.frame / 20 + (unsigned)i) % 6);
		if (((g.frame / 16 + (unsigned)i) % 5) != 0)
			fill_rect(x, y, 3, 3, GLOW);
	}
	draw_centred(64, "GLOWBUG", GLOW, 6);
	draw_centred(116, "GARDEN", rgb(120, 200, 90), 6);
	draw_sprite_scaled(&spr_firefly_0, SCREEN_W / 2 - 24, 176, 3);

	draw_centred(236, "LIGHT EVERY SEED OF THE GARDEN", GREY, 2);
	draw_sprite(&spr_ember_0, 168, 262);
	draw_sprite(&spr_moss_0, 190, 262);
	draw_sprite(&spr_dusk_0, 212, 262);
	draw_text(240, 266, "SHADOW BEETLES HUNT YOU", WHITE, 2);
	draw_sprite(&spr_flower, 190, 290);
	draw_text(240, 294, "LANTERN FLOWER: THEY FLEE", WHITE, 2);
	draw_sprite(&spr_scared_0, 190, 318);
	draw_text(240, 322, "CATCH THEM: 200 400 800 1600", WHITE, 2);

	if ((g.frame / 30) & 1)
		draw_centred(362, "PRESS START", WHITE, 3);
	draw_centred(410, music_enabled() ? "B / B2: MUSIC ON" : "B / B2: MUSIC OFF", GREY, 2);
	draw_centred(438, "STICK: FLY THE PATHS", GREY, 2);
}

/* ---- the loop ------------------------------------------------------------------------- */

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

		/* the way asked: the last pressed, else the one held */
		static const uint16_t bits[4] = { IN_RIGHT, IN_DOWN, IN_LEFT, IN_UP };
		for (int d = 0; d < 4; d++)
			if (in & bits[d])
				g.want = d;
		for (int d = 0; d < 4; d++)
			if (pressed & bits[d])
				g.want = d;

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
			if (--g.timer == 0)
				g.state = PLAY;
			break;
		case PLAY:
			play();
			break;
		case CAUGHT:
			if (--g.timer == 0) {
				if (--g.lives == 0) {
					g.state = OVER;
					g.timer = 240;
					sound_play(V_JINGLE, SND_OVER, 1024, 13, 0);
					record_keep();
				} else {
					place_movers();
					g.state = READY;
					g.timer = 90;
					music_play(MUSIC_PLAY);
				}
			}
			break;
		case CLEARED:
			if (--g.timer == 0) {
				g.level++;
				if ((g.level - 1) % MAZES == 0)
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
			clear(rgb(6, 10, 18));
			draw_garden();
			if (g.state != CLEARED || (g.frame & 8))
				draw_beetles();
			draw_fly();
			if (g.pop_time)
				draw_number(MAZE_X + g.pop_x - 4, MAZE_Y + g.pop_y + 2, g.pop_points, g.pop_points >= 1000 ? 4 : 3, GLOW, 1);
			if (g.state == READY)
				draw_centred(MAZE_Y + 13 * TILE + 2, "READY!", YELLOW, 2);
			if (g.state == CLEARED)
				draw_centred(MAZE_Y + 13 * TILE + 2, "GARDEN LIT!", GLOW, 2);
			if (g.state == OVER)
				draw_centred(MAZE_Y + 13 * TILE + 2, "GAME OVER", RED, 2);
		}
		draw_scores();
		screen_flip();
	}
}
