/*
 * blinkbios_shared_voltaile.h
 *
 * This is a workaround to letting the BIOS code and user code have different `volatile` views of the
 * shared data structures. While we need some of these shared variables to be seen as volatile to the user code
 * since they can be changed by the BIOS code in the background, the BIOS code can never be interrupted by the user code
 * so it is wasteful to have it see them as volatile also.  All the unneeded reloads are not only slow, but we really need
 * to save flash.
 *
 * To make this work, the BIOS code always `#define`s `BIOS_VOLATILE_FLAG` before including any of the shared files.
 *
 */


#ifndef BLINKBIOS_SHAED_VOLATILE__H_

    #define BLINKBIOS_SHAED_VOLATILE__H_

    #ifdef BIOS_VOLATILE_FLAG

        // BIOS code sees this

        #define BIOS_VOLATILE volatile
        #define USER_VOLATILE
        #define BOTH_VOLATILE volatile

    #else

        // User code sees this

        #define BIOS_VOLATILE
        #define USER_VOLATILE volatile
        #define BOTH_VOLATILE volatile

    #endif
    
    

#endif
