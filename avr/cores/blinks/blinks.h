/*
 * blinks.h
 *
 * Created: 6/28/2017 9:58:47 PM
 *  Author: passp
 */ 


#ifndef BLINKS_H_
#define BLINKS_H_

#define F_CPU 4000000				// Default fuses are DIV8 so we boot at 1Mhz, 
									// but then we change the prescaler to get to 4Mhz by the time user code runs

#define FACE_COUNT 6				// Total number of IRLEDs 

#define BUTTON_DEBOUNCE_TICKS 10	// Number of ticks on the timer to debounce the button

#endif /* BLINKS_H_ */