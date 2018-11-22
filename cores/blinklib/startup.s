
/*
 * startup.s
 *
 * This replaces the normal C startup code with a MUCH smaller version to save space.
 *
 * You *must* specify --nostartupfiles to the liker to use this shorter startup
 *
 * You should also use this signature for your main()...

	void main(void) __attribute__ ((section (".init9"))) __attribute__((used)) __attribute__ ((naked));

	void main(void)
	{
		// Your code here
	}

	The actual function name does not matter, but it should be void and should never complete.

	You also need to turn off the inclusion of the startard startup files.

	In AS7 that is under project settings->linker


 * Created: 10/27/2018 4:05:41 PM
 *  Author: josh
 */

#include <avr/io.h>

 	.macro	vector name
	.weak	\name
	.set	\name, __vectors	; Bad interrupt vector will just jump to reset vector
	jmp	\name
	.endm

 	.section .vectors,"ax",@progbits
	.global	__vectors
	.func	__vectors
__vectors:
	jmp 	__init
	vector	__vector_1
	vector	__vector_2
	vector	__vector_3
	vector	__vector_4
	vector	__vector_5
	vector	__vector_6
	vector	__vector_7
	vector	__vector_8
	vector	__vector_9
	vector	__vector_10
	vector	__vector_11
	vector	__vector_12
	vector	__vector_13
	vector	__vector_14
	vector	__vector_15
	vector	__vector_16
	.endfunc


 	.text

 	.section .init2,"ax",@progbits

__init:

	// Clear the __zero_reg
	// All code code assumes r1 will always be 0
	clr r1

	// Reset the stack pointer
	// While the chip will initialize these registers
	// to the right values on reset, for now we are jumpping into this
	// code warm and so need to reset manually.
	// TODO: Make sure we always get here with reset start pointer
	//       so we can delete this init
	ldi	r28,lo8(RAMEND)
	out	AVR_STACK_POINTER_LO_ADDR, r28

	ldi	r29,hi8(RAMEND)
	out	AVR_STACK_POINTER_HI_ADDR, r29

