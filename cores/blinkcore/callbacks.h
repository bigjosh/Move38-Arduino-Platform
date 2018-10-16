// This header defines the callbacks that will be provided by higher layers
// on top of this HAL layer

// There are two timer callback speeds - every 256us and every 512us.
// The 256us timer callback is broken into two parts. One is called with interrupts off
// and should complete any atomic operations very quickly and return. Then the interrupts
// on callback is called and can take longer but must comeplete before the next firing.

// User supplied callback. Called every 256us with interrupts off. Should complete work as
// quickly as possible!!!
// Actually called from pixel.cpp since we also use the pixel timer for time keeping

#ifndef CALLBACKS_H_
#define CALLBACKS_H_


void timer_128us_callback_cli(void);


// User supplied callback. Called every 256us with interrupts on. Should complete work in <<256us.
// Actually called form pixel.cpp since we also use the pixel timer for time keeping

void timer_128us_callback_sei(void);

// User supplied callback. Called every 512us with interrupts on. Should complete work in <<256us.
// Actually called from pixel.cpp since we also use the pixel timer for time keeping

void timer_256us_callback_sei(void);

// Called repeatedly after hardware has been initialized.

void run(void);

#endif