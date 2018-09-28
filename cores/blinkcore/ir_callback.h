/*
 * ir_callback.h
 *
 * This head file connects the overflow ISR in Timer.cpp to the periodic IR decoder.
 *
 * I can't think of a more elegant way to do this without the overhead of a functrion pointer, can you?
 * This only needs to be included in timer.cpp and ir.cpp.
 *
 * THis makes me think that IR decode should be handled at the next higher layer.
 */

#ifndef IR_CALLBACK_H_
#define IR_CALLBACK_H_

void ir_tick_isr_callback(void);

#endif /* IR-CALLBACK_H_ */
