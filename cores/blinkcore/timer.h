/*
 * timer.h
 *
 * Timer related functions
 *
 * Note that the RGB pixel PWM uses timer0 and timer2, so we piggy back on timer0 overflow to
 * provide the timer callback. That means all this code lives in pixel.h
 *
 */ 

#ifndef TIMER_H_
#define TIMER_H_

#include "blinkcore.h"

// These values are based on how we actually program the timer registers in timer_enable()
// There are checked with assertion there, so don't change these without changing the actual registers first

// User supplied callback. Called every 512us with interrupts on. Should complete work in <<256us.
// Actually called form pixel.cpp since we also use the pixel timer for time keeping

void timer_callback(void);

#define TIMER_PRESCALER 8       // How much we divide the F_CPU by to get the timer0 frequency

#define TIMER_TOP 256           // How many timer ticks per overflow?

#define TIMER_PHASE_COUNT  5    // How many phases between timer callback? 

#define TIMER_CYCLES_PER_TICK (TIMER_PRESCALER*TIMER_TOP)

#define F_TIMER ( F_CPU / TIMER_CYCLES_PER_TICK )       // Timer overflow frequency. Units of TICKS/SEC. Comes out to 1953.125 = 1.953125 kilohertz, 512us per tick

#define MILLIS_PER_SECOND 1000

#define SECONDS_PER_MINUTE  60

#define CYCLES_PER_MS (F_CPU/MILLIS_PER_SECOND)

#define TIMER_MS_TO_TICKS(ms)  ( ( CYCLES_PER_MS * ms)   / TIMER_CYCLES_PER_TICK )  // Slightly contorted to preserve precision

#define US_PER_SECOND 1000000

#define CYCLES_PER_US (F_CPU/US_PER_SECOND)

#define US_TO_CYCLES(us) (us * CYCLES_PER_US )
      
#endif /* TIMER_H_ */