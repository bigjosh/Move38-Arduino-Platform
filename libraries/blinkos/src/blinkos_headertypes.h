// Here are the definitions of the first header byte in a blinkOS IR packet transmision

// These header bytes are chosen to try and give some error robustness.
// So, for example, a header with a repeating pattern would be less robust
// because it is possible something blinking in the environment might replicate it

// Obviously these must all be mutually exclusive. 

// Viral game transmission related...

#define IR_PACKET_HEADER_PULLREQUEST     0b01101010      // If you get this, then the other side is saying they want to send you a game
#define IR_PACKET_HEADER_PULLFLASH       0b01011101      // You send this to request the next block of a game
#define IR_PACKET_HEADER_PUSHFLASH       0b01011101      // This contains a block of flash code to be programmed into the active area

// Higher levels

#define IR_PACKET_HEADER_USERDATA        0b11010011      // Pass up to userland




