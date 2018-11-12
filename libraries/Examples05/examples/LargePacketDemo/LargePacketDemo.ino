// SplatShare
// Demonstrates how to send long packets from the blinklib API
// Pressing the button scrambles the color pattern on the RGB LEDs
// When the button is released, the scramble is frozen and then the color
// pattern is shared to neighboors using a packet that contains the RGB
// values for each of the 6 LEDs. 

// Our currently displayed colors and also the colors we send/receive in a packet to share

static Color colors[FACE_COUNT] = { RED, GREEN, BLUE, ORANGE , YELLOW , CYAN };

byte randomByte() {

    return random(255);

}

// Generate a random color

Color randomColor() {

    return makeColorHSB( randomByte() , 255 , 255 );

}


// Assign a random color to each face

void splat() {

    FOREACH_FACE(f){
        colors[f] = randomColor();
    }

}

// Display the currently programmed colors

void updateDisplayColors() {

    FOREACH_FACE(f) {
        setColorOnFace( colors[ f ] , f );
    }

}



void setup() {
    //splat();
    updateDisplayColors();
}

// Packet handshaking
// The blink that wants to send a packet starts sending the PACKET_READY_TO_GO
// The blink that sees the PACKET_READY_TO_GO replies with a SEND_ME_THE_PACKET
// The original sending packet now replies with the actual packet. Note that you want to do this as soon as you see the SEND_ME_THE_PACKET come in.
// Once the receiving blink gets the packet, it stops sending the SEND_ME_THE_PACKET and the transfer is complete!

// You might not need to have explicit command for this if the game happens to already have state changes that are natural places
// to send and ACK the packets, but in this demo they are needed to keep the transmissions ping ponging.

#define I_GOT_THE_PACKET    63     // Signal that we got the packet so they can stop sending.
#define IDLE_STATE           0     // Go back to some other state. In a real program this would be lots of other normal states.

/*

    Here is how the packet send dance works...

    T wants to send a packet to R.

    T sends the packet until it gets an I_GOT_THE_PACKET from R.

    When R gets a packet, it sends I_GOT_THE_PACKET until it sees something that is not a packet from T.

    Since this API presents a state-based model, the right way to do this would be to treat the packets like all other face vales.
    You could call setValueOnFace() with a byte, or an array of bytes and the blink would continuously send whatever you set and the other side
    we see with getLastValueReceivedonFace().

    Unfortunately this just doesn't work on this resource constrained platform because each face would need about 100 bytes of extra buffers
    for this to work. It would also be butt slow to keep sending these very long messages continuously and we would loose our required
    10Hz refresh rate.

    So instead we basically have to build an event based model on top of the state based model (which is itself built on an event model!).
    Those PACKET-based states are not really states, it is the transitions between the states that simulate events.

    Things are much simpler in an event-driven model, and maybe games that need big packets should use the event driven API. The state
    API is probably best for games like the old school ones that really are state based.

*/


// The game state just a stand-in for this demo, but in a normal program this would be the normal
// state values that get sent when no packet transfer activity is happening.
// It can use any values except the one we reserved above for packet transfer states.

static byte current_game_state = IDLE_STATE;

static bool pending_packet_send_on_face[FACE_COUNT];
static bool pending_ack_send_on_face[FACE_COUNT];

void loop() {

    FOREACH_FACE(f) {

        if ( !isValueReceivedOnFaceExpired(f)) {

            byte value_received=getLastValueReceivedOnFace(f);

            if ( value_received == I_GOT_THE_PACKET ) {

                pending_packet_send_on_face[f] = false;

            } else {

                pending_ack_send_on_face[f] = false;

            }

            if (pending_packet_send_on_face[f]) {                        // Do we have something to send on this face?

                // Note that we do not care if this succeeded or not since we will keep sending until
                // we get an ACK back.

                sendPacketOnFace( f , (byte *) colors , sizeof(colors) );

                // Keep in mind that we do not change the value send here, so in case the other side misses this packet
                // then we will send again when we get their next SEND_ME_THE_PACKET in response to seeing our I_HAVE_A_PACKET_4_U

            }

        }   // !isValueReceivedOnFaceExpired(f)

    }

    // Single button press re-splats our colors and signals to neighbors we have something for them

    if ( buttonDown() ) {

        // Keep scrambling while button is down

        splat();
        updateDisplayColors();

    }

    if (buttonReleased()) {

        // When the button is released, the scramble stops and it is time to send the update out

        FOREACH_FACE(f) pending_packet_send_on_face[f]=true;        // This will trigger a send on all faces

    }

    // Deal with any packets that came in to us

    FOREACH_FACE(f) {

        if ( isPacketReadyOnFace( f ) )  {

            if ( getPacketLengthOnFace(f) == sizeof ( colors ) ) {      // Just a double check to make sure the packet is the right length. Just stops corrupt packets or packets from other games from getting into our headspace.

                Color *receivedColors = (Color *) getPacketDataOnFace( f ) ;

                FOREACH_FACE( g ) {

                    colors[g] = receivedColors[g];

                }

                updateDisplayColors();

            }

            pending_ack_send_on_face[f]=true;           // Tell other side that we got it so they can stop sending it

        }       // if ( isPacketReadyOnFace( f ) )  {

    }            //  FOREACH_FACE(f) {

    // Now finally set the outgoing values on each face to reflect the packet state when nessisary

    FOREACH_FACE(f) {

        if (pending_ack_send_on_face[f]) {

            setValueSentOnFace( I_GOT_THE_PACKET , f );

        } else {

            setValueSentOnFace( current_game_state , f );

        }
    }       // FOREACH_FACE

}           // loop()

