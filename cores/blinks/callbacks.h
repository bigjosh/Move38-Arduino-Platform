/*
 * callbacks.h
 * 
 * The user provides these functions to be called by the platform. 
 *
 */ 

#ifndef CALLBACKS_H_
#define CALLBACKS_H_


// Called after initial power up complete. 
// if run() returns, it will be immedeately called again. 

void run(void);

#endif /* CALLBACKS_H_ */