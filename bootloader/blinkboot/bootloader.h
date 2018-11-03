/*
 * blinkboot.h
 *
 * This defines stuff that is needed to transfer games to the bootloader
 *
 */

#define FLASH_PAGE_SIZE 128      // Flash pages size for ATMEGA168PB
                                    // We currently send in full pages because it save the hassle of reassembling packets, but does mean
                                    // that we must have bigger buffers. Maybe makes sense to send blocks instead?

#define DOWNLOAD_MAX_PAGES 56       // The maximum length of a downloaded game
                                    // Currently set to use ~7KB. This saves 7KB for built in game and 2KB for bootloader

// These header bytes are chosen to try and give some error robustness.
// So, for example, a header with a repeating pattern would be less robust
// because it is possible something blinking in the environment might replicate it

#define IR_PACKET_HEADER_SEED       0b01101010      // If you get this, then the other side is saying they want to send you a game
#define IR_PACKET_HEADER_ALIVE      0b11101000      // Send to our source instead of a seed to say that we are still pushing, or someone lower than us in the tree is stil pushing.
#define IR_PACKET_HEADER_PULL       0b01011101      // You send this to request the next block of a game
#define IR_PACKET_HEADER_PUSH       0b11011011      // This contains a block of flash code to be programmed into the active area

// TODO: Add an *all done* packet the percolates down so we do not need to wait for timeouts to know when the full tree has finished downloading?

struct push_payload_t {                 // Response to a pull with the flash block we asked for
    uint8_t data[FLASH_PAGE_SIZE];   // An actual page of the flash memory
    uint8_t page;                       // The block number in this packet. This comes after the data to keep the data word aligned.
    uint8_t packet_checksum;            // Simple sum of all preceding bytes in packet including header, then inverted. This comes at the end so we can compute it on the fly.
};

struct seed_payload_t {           // Sending blink telling neighbor there is a new game he needs to download
    uint8_t pages;                        // How many total pages in this game? We put this first in case the compile wants to pad the header byte
    uint16_t program_checksum;            // The checksum of all the flash data in all of the packets with each page also has added in its page number
    uint8_t packet_checksum;            // Simple sum of all preceding bytes in packet including header, then inverted. This comes at the end so we can compute it on the fly.
};


struct pull_payload_t {                   // Blink asking neighbor for a block of a new game
    uint8_t page;                       // The block we want the neighbor to send
};


struct blinkboot_packet {

    uint8_t header;                             // One of the three above packet types

    union {

        push_payload_t          push_payload;
        seed_payload_t          seed_payload;
        pull_payload_t          pull_payload;

    };
};


/*
// Calls from application into bootloader

#define BOOTLOADER_DOWNLOAD_MODE_VECTOR __vector_1

// Call this to have the bootloader try to start downloading from specified face
// If successful, downloaded game will get started. If not, builtin game will get copied and started.

#define BOOTLOADER_DOWNLOAD_MODE_JMP(face) { R25=face; CLEAR_STACK_JMP(BOOTLOADER_DOWNLOAD_MODE_VECTOR ); }

#define BOOTLOADER_SEED_MODE_VECTOR __vector_2

// Call this to have the bootloader copy the built in game to the active area and start seeding it
// When the seeding is complete, the bootloader will jump to the active game's reset vector at 0x000

#warning For now just uses in place active game,
// TODO: Copy active game down

#define BOOTLOADER_SEED_MODE_JMP(face) { CLEAR_STACK_JMP(BOOTLOADER_SEED_MODE_VECTOR ); }
  
*/  
    
#define BOOTLOADER_RESET_ADDRESS (0x3800)       // Jump to the reset vector of the bootloader
    
#define BOOTLOADER_RESET_JMP() { CLEAR_STACK_JMP(BOOTLOADER_RESET_VECTOR ); }
        
// Temp way to start up the bootloader to start seeding the active game        
        
#define JUMP_TO_BOOTLOADER_SEED()       { GPIOR1 = 'S'; BOOTLOADER_RESET_JMP(); }
    
// Temp way to jump into the bootloader to tell it to start downloading from the next face that gets a seed packet    

#define JUMP_TO_BOOTLOADER_DOWNLOAD()   { GPIOR1 = 'D'; BOOTLOADER_RESET_JMP(); }



/*
// State for each receiving IR LED

struct ir_rx_state_t {


    // These internal variables are only updated in ISR, so don't need to be volatile.

    uint8_t windowsSinceLastTrigger;                // How long since we last saw a trigger on this IR LED?


    // We add new samples to the bottom and shift up.
    // The we look at the pattern to detect data bits
    // This is better than using a counter because with a counter we would need
    // to check for overflow before incrementing. With a shift register like this,
    // 1's just fall off the top automatically and We can keep shifting 0's forever.

    uint8_t byteBuffer;                             // Buffer for RX byte in progress. Data bits shift up until full byte received.
    // We prime with a '1' in the bottom bit when we get a valid start.
    // This way we can test for 0 to see if currently receiving a byte, and
    // we can also test for '1' in top position to see if full byte received.


    uint8_t packetBuffer[ IR_RX_PACKET_SIZE];        // Assemble incoming packet here
    // TODO: Deeper data buffer here?

    uint8_t packetBufferLen;                         // How many bytes currently in the packet buffer?

    volatile uint8_t packetBufferReady;              // 1 if we got the trailing sync byte. Foreground reader will set this back to 0 to enable next read.

    // This struct should be even power of 2 in length for more efficient array indexing.

} ;


  */