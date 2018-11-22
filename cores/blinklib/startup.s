
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



 	.text

 	.section .init2,"ax",@progbits

__init:

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