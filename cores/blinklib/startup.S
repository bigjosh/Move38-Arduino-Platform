
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


/*

	There is nothing here! NO VECTORS here! No __zero_reg! We just jump stright into the code!

	This saves a lot of flash becuase each game can start litterally at address 0x000 and hit the
	ground running with no extra crap at the begining.

	The vectors are up in the bootloader area. Code compiled against this blinklib API
	is depenent on that BlinkBIOS code running at reset and setting everythign up for us
	inclduing the interrupts.

	Note that we do need to init the .data and .bss variables to thier initial values, but that
	is taken care of by library code that will link in here automatically.

*/

	// There does not seem to be any good way to avoid this jmp since
	// the compiler likes to put the contructors and jump trampolines at the bottom of flash,
	// which ended up being 0x0000 when we get rid of all the vectors!


	.section .vectors,"ax",@progbits
	.global	__vectors
	.func	__vectors
__vectors:
	jmp __init
	.endfunc


	 .section .init0,"ax",@progbits
     .func   __init
__init:


	.section .init2,"ax",@progbits

	//eor r1, r1		// The boot loader always does this for us, so dont bother repeating
						// We also always get a clean stack

	.section .init9,"ax",@progbits

	jmp main
