
#include <avr/pgmspace.h>       // Flash access functions

#include "blinkos.h"
#include "blinkos_viral.h"

#include "blinkos_timer.h"          // get US_TO_CYCLES()

#include "blinkos_irdata.h"
#include "blinkos_viral.h"

#include "blinkos_pixel.h"
#include "pixelcolor.h"

// TODO: Move all these variables into local so they don't use space unless we are actually sending.

uint16_t game_checksum;

#define TIME_BETWEEN_PULL_REQUESTS_MS 100

static inline void compute_game_checksum() {
        
    uint16_t *ptr = (uint16_t *) (DOWNLOAD_PAGE_SIZE * DOWNLOAD_MAX_PAGES);
    
    // Counting down slightly faster than counting up
    // TODO: Bet this could be even faster in ASM because of RO:R1 nature of the flash read functions
        
    while (ptr--) {
        
        game_checksum+= pgm_read_dword_near( ptr );
    }        
    
}    

static inline void send_pull_request( uint8_t face ) {
    
    if (irSendBegin( face )) {
        
        irSendByte( IR_PACKET_HEADER_PULLREQUEST );
        irSendByte( DOWNLOAD_MAX_PAGES  );             // total length of game in pages
        irSendByte( DOWNLOAD_MAX_PAGES  );             // Highest page currently available from me
        
        irSendByte( game_checksum & 0xff );            // Checksum low byte
        irSendByte( game_checksum >> 8 );              // Checksum high byte
        
        void irSendComplete();
        
    }    
    
}    


void blinkos_transmit_self() {
    
    // First compute our checksum
    
    compute_game_checksum();
    
    OS_Timer next_pull_request_transmition;
    
    if ( next_pull_request_transmition.isExpired() ) {
        
        // Time to send a pull request out to get people to start pulling from us
        
        
        OS_FOREACH_FACE(f) {
            
            blinkos_pixel_bufferedSetPixel( f , pixelColor_t( 0 , 255 , 0 , 1 ) );
            blinkos_pixel_displayBufferedPixels();
            
            send_pull_request(f);
            
        }            
        
        next_pull_request_transmition.set( TIME_BETWEEN_PULL_REQUESTS_MS );        
        
        OS_FOREACH_FACE(f) {
            blinkos_pixel_bufferedSetPixel( f , pixelColor_t( 0 , 0 , 5 , 1 ) );                        
        }            
        
        blinkos_pixel_displayBufferedPixels();        
        
    }        
    
    
    
}   