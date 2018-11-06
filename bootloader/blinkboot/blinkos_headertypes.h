// Here are the definitions of the first header byte in a blinkOS IR packet transmision

// These header bytes are chosen to try and give some error robustness.
// So, for example, a header with a repeating pattern would be less robust
// because it is possible something blinking in the environment might replicate it


// Use an enum here because it will prevent duplicate header bytes from creeping in
// besides avoiding duplicates, also try to maximize the hamming distance between these big

namespace ir_packet_header_enum {

    enum {

        SEED    =   0b01101010,      // If you get this, then the other side is saying they want to send you a game
        PULL    =   0b01011101,      // You send this to request the next block of a game
        PUSH    =   0b11011011,      // This contains a block of flash code to be programmed into the active area
        ACTIVE  =   0b11101000,      // Send to our source any time we get a valid pull or active packet. Basically this is an empty pull.
        LETSGO  =   0b00101000,      // A viral packet initiated by the root of a download when he sees everyone else has finished
        USERDATA=   0b00110111,      // Used by blink OS to carry user data packets
    };
};

// TODO: Add an *all done* packet the percolates down so we do not need to wait for timeouts to know when the full tree has finished downloading?

#define FLASH_PAGE_SIZE SPM_PAGESIZE    // Fixed for a given processor

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

