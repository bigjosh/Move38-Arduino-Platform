// Handles game transmision
// Note that game reception is handled in the bootloader


// TODO: These should get moved to a file that is shared between blinkboot and blinkos

#define DOWNLOAD_MAX_PAGES 56       // The maximum length of a downloaded game.
                                    // The represents the length of the active game area, which is 0x000 to 0x06ff
                                    // (remember those are words not bytes so this is 0x700 * 2 = )


#define DOWNLOAD_PAGE_SIZE 128                // Flash pages size for ATMEGA168PB

// These header bytes are chosen to try and give some error robustness.
// So, for example, a header with a repeating pattern would be less robust
// because it is possible something blinking in the environment might replicate it

#define IR_PACKET_HEADER_PULLREQUEST     0b01101010      // If you get this, then the other side is saying they want to send you a game
#define IR_PACKET_HEADER_PULLFLASH       0b01011101      // You send this to request the next block of a game
#define IR_PACKET_HEADER_PUSHFLASH       0b01011101      // This contains a block of flash code to be programmed into the active area


// Start sending ourselves out
// TODO: returns (reboots?) when all faces are sending PR requests that match our own game


void blinkos_transmit_self();