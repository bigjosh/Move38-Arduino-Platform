/*
 * hardware.h
 *
 * Defines the location of all the used hardware
 *
 * Created: 7/23/2017 9:50:54 PM
 *  Author: passp
 */ 


/*

    There are currently two hardware versions of the board in the wild and they
    have slightly different pin assignments.
    
    For now, we control which one we are compiling to by selective inclusion here.
    
    In the future, hopefully we can abandon the V1 and have a single hardware.h.
    
    If it turns out we need to support both versions, I bet we could detect version at runtime
    but I don't want to spend time on that now. 
   
*/


#ifdef PCB_V2


    // For newer boards. So far, only I have these so breaking this 
    // out with a CLI #define to not impact others pulling this repo
    
    #include "hardware_V2.h"

#else

    #include "hardware_V1.h"

#endif

