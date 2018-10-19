/*
 * bareblinkOS.cpp
 *
 * An example that runs directly on the blinkOS without blinklib
 *
 */


#include "blinkos.h"


void setupEntry() {

    asm("nop");

};

uint8_t step;
millis_t nextstep;

millis_t nextsend;

uint8_t message[] = { 'J' , 'o' , 's' , 'h' };

void loopEntry( loopstate_in_t const *loopstate_in , loopstate_out_t *loopstate_out) {

    millis_t now = loopstate_in->millis;

    if ( loopstate_in->buttonstate.bitflags & BUTTON_BITFLAG_PRESSED ) {

        loopstate_out->colors[step] =  pixelColor_t( 0 , 0 , 0 , 1 );

        step++;

        if (step>= PIXEL_FACE_COUNT ) {
            step=0;
        }

        loopstate_out->colors[step] = pixelColor_t( 23 , 10, 0 , 1 );

        nextstep = loopstate_in->millis + 200;
    }


    for( uint8_t f=0; f< IR_FACE_COUNT; f++ ){

        if ( loopstate_in->ir_data_buffers[f].ready_flag ) {

            if ( (loopstate_in->ir_data_buffers[f].len == 4 )) {

                const uint8_t *d = loopstate_in->ir_data_buffers[f].data;

                if ( d[0] =='J' && d[1] =='o' && d[2] == 's' && d[3] == 'h' ) {

                    // Good packet!
                    loopstate_out->colors[f] = pixelColor_t( 0 , 20, 0 , 1 );

                } else {

                    // Good length, bad data!
                    loopstate_out->colors[f] = pixelColor_t( 20 , 0, 0 , 1 );

                }

            } else {

                // Bad len
                loopstate_out->colors[f] = pixelColor_t( 0 , 0, 20 , 1 );

            }

        }

    }
    
 

    if ( nextsend < now ) {

        ir_send_userdata( 4 , message , 4 );

        nextsend = now + 200;
    }


}    