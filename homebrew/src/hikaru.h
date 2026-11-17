/*
 * hikaru.h: what a homebrew needs of the SEGA Hikaru, as Odin emulates it
 *
 * Copyright (C) 2026 the odin-android contributors
 * GPL 3 or later, as Odin.
 */
#ifndef HIKARU_H
#define HIKARU_H

#include <stdint.h>

#define REG32(a) (*(volatile uint32_t *)(a))
#define REG16(a) (*(volatile uint16_t *)(a))

/* ---- 2D layer 1: a 640 x 480 picture in the framebuffer RAM ----------- */

#define SCREEN_W 640
#define SCREEN_H 480

/* ABGR1555, the alpha bit set (opaque) */
static inline uint16_t rgb(unsigned r, unsigned g, unsigned b)
{
	return (uint16_t)(0x8000 | ((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3));
}

/* Layer 1 shown above the 3D, opaque. Two pictures in the framebuffer
 * RAM: one shown while the other is drawn (screen_flip swaps them). */
void layer_init(void);

/* The picture being drawn: SCREEN_H lines of SCREEN_PITCH pixels */
#define SCREEN_PITCH 2048
extern volatile uint16_t *screen;

/* Waits for the start of the next frame (the line counter of the display
 * chip going back to the top) */
void wait_frame(void);
/* Shows the picture drawn from the next frame on; the other one is then
 * the one drawn */
void screen_flip(void);

/* ---- inputs: JVS through the MIE ---------------------------------------- */

/* The switches of player 1 (JVS): the first byte in the high byte */
enum {
	IN_START = 0x8000, IN_SERVICE = 0x4000,
	IN_UP = 0x2000, IN_DOWN = 0x1000, IN_LEFT = 0x0800, IN_RIGHT = 0x0400,
	IN_B1 = 0x0200, IN_B2 = 0x0100, IN_B3 = 0x0080, IN_B4 = 0x0040,
	IN_B5 = 0x0020, IN_B6 = 0x0010,
};

/* Starts the JVS program of the MIE (the one of the cabinet's BOOTROM,
 * copied from it at run time: none of SEGA's code is in this ROM) */
void input_init(void);
/* Whether the JVS board answers yet (some ten seconds after input_init) */
int input_ready(void);
/* The switches of player 1 now: a JVS poll, its answer waited for (some 4 ms) */
uint16_t input_read(void);
/* The Test switch of the cabinet */
int input_test(void);

/* ---- the backup RAM: 64 KB on a battery, kept by Odin ------------------ */

/* Bytes of it, from `offset` (0 to 0xFFFF) */
void backup_read(unsigned offset, void *to, unsigned size);
void backup_write(unsigned offset, const void *from, unsigned size);

#endif
