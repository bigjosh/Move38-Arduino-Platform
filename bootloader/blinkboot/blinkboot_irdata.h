 // Called from timer on every click to process newly received IR LED levels


 // IR Packet functions
 // Note that there is no error checking on packet data. Packets must be well formed, which means that
 // they must start and end with SYNC and have only a multiple of 8 valid bits in between the SYNCs.
 // If there is any structural error while receiving a potential packet, then it is aborted and we
 // start looking for a new one.

// Maximum IR packet size. Larger values more efficient for large block transfers, but use more memory.
// Packets larger than this are silently discarded.
// Note that we allocate 6 of these, so memory usage goes up fast with bigger packets sizes.

#define IR_RX_PACKET_SIZE     133

void IrDataPeriodicUpdateComs(void);

// Is there a received data ready to be read on this face?

bool irDataIsPacketReady( uint8_t led );


// Returns length of the  data if complete packet is ready (note that zero is valid)

uint8_t irDataPacketLen( uint8_t led );


// Get a handle to the received packet buffer.
// Note only valid after irDataIsPacketReady() turns true and before irDataMarkPacketRead() is called.

const void *irDataPacketBuffer( uint8_t led );


// Mark most recently received packet as read, freeing up the buffer to receive the next packet.
// Note that new packets that start coming in before this is called are sileintly discarded.

void irDataMarkPacketRead( uint8_t led );


 // Sends bits in least-significant-bit first order (RS232 style)

 // Note that you must not dilly dally between begin, sending bytes, and ending. You don't have much time to think
 // so get everything set up beforehand.

 // Returns 1 if the send was successfully started, 0 if there was an RX in progroess (you should then try again)

 uint8_t irSendBegin( uint8_t face );

 void irSendByte( uint8_t b );

 void irSendComplete();

 void irDataInit();   // Really only called to init IR_RX_DEBUG
