/* crt0.S: where the BOOTROM jumps (the end entry of the load table) */
	.section .text.start
	.global _start
_start:
	mov.l	stack, r15
	/* .bss to zero */
	mov.l	bss_from, r1
	mov.l	bss_to, r2
	mov	#0, r0
1:	cmp/hs	r2, r1
	bt	2f
	mov.l	r0, @r1
	bra	1b
	add	#4, r1
2:	mov.l	main_at, r0
	jsr	@r0
	nop
3:	bra	3b
	nop
	.align 2
stack:		.long	0x8C7FFF00
bss_from:	.long	__bss_start
bss_to:		.long	__bss_end
main_at:	.long	main
