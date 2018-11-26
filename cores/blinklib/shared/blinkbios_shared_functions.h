/*
 * blinkbios_shared_functions.h
 *
 * Defines the entry points into the BLINKBIOS called functions
 *
 */

// Note: Aesthetically it might seem better to put these function declarations with their functional groups, but ultimately I decided to
//       keep them all here in one place to avoid vector collisions. It is not a decision without downside though since a client who, say,
//       only wants the IR send funtion gets polluted with the others as well. :/

// The VECTOR definitions are used on the BIOS side to hook the correct incoming vector.
// The function definitions are used on the caller side to make the actual call to the vector.
// The vector number and the jmp address must be manually matched.

// The actual vector number is not important as long as it is not used for an actual ISR.  We really just use the vector pattern so the
// caller has a well known address to jmp to and vectors are always at a well known address.

// The `jmp`s just work because the caller will set up the args in the right registers and then make the jmp to the
// interrupt vector. Once there, the vector will directly call the target function with all the args still in place,
// then the target will return back to the original caller. Cool right? The optimizer will even put the target function at
// the vector jump address if it has no other callers, saving a jmp.

#ifndef BLINKBIOS_SHARED_FUCNTIONS_H_
#define BLINKBIOS_SHARED_FUCNTIONS_H_


#warning Figure these out
/*
#define BLINKBIOS_BURN_FLASH_PAGE_VECTOR    __vector_8      // This lands at base + 4 bytes per vector * 3rd vector (init is at 0) = 0x0c

inline void __attribute__((naked)) blinkbios_burn_flash_page(  uint8_t face, const uint8_t *data , uint8_t len )  {
    asm("jmp 0x380C" );
}

// Copy the built in game to the active area

#define BLINKBIOS_LOAD_BUILTIN    __vector_1                   // This lands at base + 4 bytes per vector * 1st vector (init is at 0) = 0x04

inline void __attribute__((naked)) blinkbios_load_builtin(void)  {
    asm("jmp 0x3804" );
}
*/

// Send a user data packet
// See what we did here - we do a naked jump into vector_4, which is a jump to the `uint8_t ir_send_userdata( uint8_t face, const uint8_t *data , uint8_t len )` function
// it all works out because the params happened to be in the same registers because of the AVR C calling convention.
//  When compiling the BIOS with LTO, it even puts the send packet function right at the target of the vector.

// We use unused inetrrupt vectors to link between the user and BIOS code since they are at a known place.
// The links are defined as symbols like `boot_vectorX` where X is the number of the unused vector we are taking. 
// In the BIOS project, these appear in the vector table in `startup.S`.
// IN the user mode projects, these appear in the linkscript and are hard coded to the correct addressed (based at the bootloader vtable at 0x3800). 

// boot_vector4 is defined in the linkerscript and points to the boot loader's interrupt vector 4 at address 0x3810
// This is just a prototype so gcc knows what args to pass. The linker will resolve it to a jump to the address.

#define BLINKBIOS_IRDATA_SEND_PACKET_VECTOR boot_vector4      // This lands at base + 4 bytes per vector * 4th vector (init is at 0) = 0x10

extern "C" uint8_t BLINKBIOS_IRDATA_SEND_PACKET_VECTOR(  uint8_t face, const uint8_t *data , uint8_t len );

#endif /* BLINKBIOS_SHARED_FUCNTIONS_H_ */