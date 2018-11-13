/*
 * blinkboot.h
 *
 * This defines stuff that is needed to transfer games to the bootloader
 *
 */


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


// To tell the bootlader what we want it to do, we have limited options since it will wipe RAM
// on entry and the stack get cleared, so instead we use the spare GPIOR register.

#define BOOTLOADER_GPOIR_SEED_MODE      'S'
#define BOOTLOADER_GPOIR_DOWNLOAD_MODE  'D'

// Appears you can not put anything but straight test in the ASM string, so 0x3800 hardcoded in

#define BOOTLOADER_RESET_JMP() asm("jmp 0x3800")

// Temp way to start up the bootloader to start seeding the active game

#define JUMP_TO_BOOTLOADER_SEED()       { GPIOR1 = BOOTLOADER_GPOIR_SEED_MODE; cli(); BOOTLOADER_RESET_JMP(); }

// Temp way to jump into the bootloader to tell it to start downloading from the next face that gets a seed packet

#define JUMP_TO_BOOTLOADER_DOWNLOAD()   { GPIOR1 = BOOTLOADER_GPOIR_DOWNLOAD_MODE; cli(); BOOTLOADER_RESET_JMP(); }


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