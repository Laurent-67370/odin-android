/* The program in the master's RAM, cached (P1), above what the BOOTROM uses */
ENTRY(_start)
SECTIONS {
	. = 0x8C100000;
	.text : { *(.text.start) *(.text*) }
	.rodata : { *(.rodata*) }
	.data : { *(.data*) }
	. = ALIGN(4);
	__bss_start = .;
	.bss : { *(.bss*) *(COMMON) }
	. = ALIGN(4);
	__bss_end = .;
}
