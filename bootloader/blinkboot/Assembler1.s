
/*
 * Assembler1.s
 *
 * Created: 10/27/2018 4:05:41 PM
 *  Author: passp
 */

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
	.global	__bad_interrupt
	.func	__bad_interrupt
__bad_interrupt:
	.weak	__vector_default
	.set	__vector_default, __vectors
	jmp	__vector_default
	.endfunc


 	.text

 	.section .init2,"ax",@progbits

__init:

	clr r1

