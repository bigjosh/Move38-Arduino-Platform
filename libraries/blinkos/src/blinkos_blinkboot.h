// Support functions for being a bootloader server (able to send the current active game)


void blinkos_blinkboot_setup();

void blinkos_blinkboot_loop();


// Send response to a pull message that contains one page of flash memory
// Called from the IR packet handler when it sees the PULL header in an incoming message

void blinkos_blinkboot_sendPushFlash( uint8_t f , uint8_t page );


// Tell a neighboring tile to start download a game from us using pull messages

uint8_t blinkos_blinkboot_sendPullRequest( uint8_t face );

        
