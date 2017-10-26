/*
 * Timer related functions
 * 
 */ 


#include "hardware.h"
#include "blinkcore.h"

#include <util/delay.h>

#include "debug.h"

#include "power.h"

#include "pixel.h"          // We depend on the pixel ISR to call us for timekeeping

#include "callbacks.h"

#include "timer.h"
