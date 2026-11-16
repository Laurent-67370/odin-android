/*
 * hikaru.c: the board, as hikaru.h tells it
 *
 * Copyright (C) 2026 the odin-android contributors
 * GPL 3 or later, as Odin.
 *
 * Sources: Odin's emulation of the board (src/odin/machine/chips/eurasia,
 * boards/hikaru/hikaru_bus.cpp, mie_board.cpp), and what the BOOTROM does
 * as Odin runs it.
 */
#include "hikaru.h"

/* ---- 2D layer ------------------------------------------------------------ */

#define FB        0xBB000000u	/* framebuffer RAM 1B000000, uncached */
#define FB_PITCH  4096u		/* a line of the RAM, 2048 x 2 bytes */
/* The two pictures of layer 1, below the 3D framebuffer */
static const unsigned picture_y[2] = { 1024u, 1536u };
static unsigned drawn;		/* the picture being drawn */
volatile uint16_t *screen;

/* Unit 0 of the display chip (315-6087): layer 1 corners, formats, enable,
 * priority, transparency */
#define LAYER1_LO   0xBA000190u
#define LAYER1_HI   0xBA000194u
#define LAYER_FMT   0xBA0001B0u
#define LAYER_ON    0xBA0001B4u
#define LAYER_ABOVE 0xBA0001B8u
#define LAYER_KEY   0xBA0001BCu
#define RASTER      0xBA00001Cu	/* bits 11-21: the line drawn */

static void show(unsigned y)
{
	REG32(LAYER1_LO) = (y << 9) | 0;
	REG32(LAYER1_HI) = ((y + SCREEN_H) << 9) | (SCREEN_W / 2);
}

void layer_init(void)
{
	drawn = 1;
	screen = (volatile uint16_t *)(FB + picture_y[drawn] * FB_PITCH);
	show(picture_y[0]);
	REG32(LAYER_FMT) = 0;		/* ABGR1555 */
	REG32(LAYER_KEY) = 0;		/* opaque */
	REG32(LAYER_ABOVE) = 1;		/* above the 3D */
	REG32(LAYER_ON) = 1;
}

void screen_flip(void)
{
	/* The chip takes the corners of the layer as a frame starts */
	show(picture_y[drawn]);
	wait_frame();
	drawn ^= 1;
	screen = (volatile uint16_t *)(FB + picture_y[drawn] * FB_PITCH);
}

void wait_frame(void)
{
	unsigned last = (REG32(RASTER) >> 11) & 0x7FF;

	for (;;) {
		const unsigned line = (REG32(RASTER) >> 11) & 0x7FF;
		if (line < last)
			return;
		last = line;
	}
}

/* ---- MIE ----------------------------------------------------------------- */

/* The Z80 memory of the MIE: byte n at 00820000 + 2n */
#define ZMEM(a) (*(volatile uint8_t *)(0xA0820000u + 2u * (a)))
/* The bus request of the Z80 */
#define BUSRQ_TAKE 0xA1000000u
#define BUSRQ_GIVE 0xA1000100u

/* Where the BOOTROM keeps its JVS program for the MIE (0.96) */
#define BOOT_JVS      0xA0040C48u
#define BOOT_JVS_SIZE 0x90Cu
#define BOOT_JVS_TAIL 14u

static void bus_take(void)
{
	REG16(BUSRQ_TAKE) = 0x100;
	while (REG16(BUSRQ_TAKE) & 0x100)
		REG16(BUSRQ_TAKE) = 0x100;
}

static void bus_give(void)
{
	REG16(BUSRQ_GIVE) = 0;
	while (!(REG16(BUSRQ_GIVE) & 0x100))
		REG16(BUSRQ_GIVE) = 0;
}

void input_init(void)
{
	const volatile uint8_t *from = (const volatile uint8_t *)BOOT_JVS;

	bus_take();
	for (unsigned i = 0; i < BOOT_JVS_SIZE; i++)
		ZMEM(0x8000 + i) = *from++;
	for (unsigned i = 0; i < BOOT_JVS_TAIL; i++)
		ZMEM(0x8FF2 + i) = *from++;
	ZMEM(0xC001) = 0xFF;			/* busy until it has started */
	ZMEM(0x7800) = ZMEM(0x7800) | 0x80;	/* the MIE ROM jumps to 0x8010 */
	bus_give();
}

/* The mailbox at Z80 C000: C004 = 3 once the program is ready, C001 = 0
 * when it is idle; command 1 in C000 (C001 set) polls the JVS board, whose
 * switches come in E003 (the cabinet) and E004-E005 (player 1). */
static uint16_t switches;
static uint8_t cabinet;

int input_ready(void)
{
	return ZMEM(0xC004) == 3;
}

uint16_t input_read(void)
{
	if (ZMEM(0xC004) == 3 && ZMEM(0xC001) == 0) {
		cabinet = ZMEM(0xE003);
		switches = (uint16_t)(ZMEM(0xE004) << 8 | ZMEM(0xE005));
		ZMEM(0xC000) = 1;
		ZMEM(0xC001) = 1;
	}
	return switches;
}

int input_test(void)
{
	return (cabinet & 0x80) != 0;
}
