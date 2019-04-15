// SplatShare
// Demonstrates how to send long Datagrams from the blinklib API,
// including a system to make sure datagrams get delivered by resending them until an ACK message is recieved. 

// Pressing the button scrambles the color pattern on the RGB LEDs
// When the button is released, the scramble is frozen and then the color
// pattern is shared to neighboors using a Datagram that contains the RGB
// values for each of the 6 LEDs. 

// Our currently displayed colors and also the colors we send/receive in a Datagram to share

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

// Datagram handshaking
// The blink that wants to send a Datagram starts sending the Datagram_READY_TO_GO
// The blink that sees the Datagram_READY_TO_GO replies with a SEND_ME_THE_Datagram
// The original sending Datagram now replies with the actual Datagram. Note that you want to do this as soon as you see the SEND_ME_THE_Datagram come in.
// Once the receiving blink gets the Datagram, it stops sending the SEND_ME_THE_Datagram and the transfer is complete!

// You might not need to have explicit command for this if the game happens to already have state changes that are natural places
// to send and ACK the Datagrams, but in this demo they are needed to keep the transmissions ping ponging.

#define I_GOT_THE_DATAGRAM    63     // Signal that we got the Datagram so they can stop sending.
#define IDLE                   0     // Go back to some other state. In a real program this would be lots of other normal states.

/*

    Here is how the Datagram send dance works...

    T wants to send a Datagram to R.

    T sends the Datagram until it gets an I_GOT_THE_Datagram from R.

    When R gets a Datagram, it sends I_GOT_THE_Datagram until it sees something that is not a Datagram from T.

    Since this API presents a state-based model, the right way to do this would be to treat the Datagrams like all other face vales.
    You could call setValueOnFace() with a byte, or an array of bytes and the blink would continuously send whatever you set and the other side
    we see with getLastValueReceivedonFace().

    Unfortunately this just doesn't work on this resource constrained platform because each face would need about 100 bytes of extra buffers
    for this to work. It would also be butt slow to keep sending these very long messages continuously and we would loose our required
    10Hz refresh rate.

    So instead we basically have to build an event based model on top of the state based model (which is itself built on an event model!).
    Those Datagram-based states are not really states, it is the transitions between the states that simulate events.

    Things are much simpler in an event-driven model, and maybe games that need big Datagrams should use the event driven API. The state
    API is probably best for games like the old school ones that really are state based.

*/

// The game state just a stand-in for this demo, but in a normal program this would be the normal
// state values that get sent when no Datagram transfer activity is happening.
// It can use any values except the one we reserved above for Datagram transfer states.

static bool pending_Datagram_send_on_face[FACE_COUNT];
static bool pending_ack_send_on_face[FACE_COUNT];

void loop() {

    FOREACH_FACE(f) {

        if ( !isValueReceivedOnFaceExpired(f)) {

            byte value_received=getLastValueReceivedOnFace(f);

            if ( value_received == I_GOT_THE_DATAGRAM ) {

                pending_Datagram_send_on_face[f] = false;

            } else {

                pending_ack_send_on_face[f] = false;

            }

            if (pending_Datagram_send_on_face[f]) {                        // Do we have something to send on this face?

                // Note that we do not care if this succeeded or not since we will keep sending until
                // we get an ACK back.

                sendDatagramOnFace( f , (byte *) colors , sizeof(colors) );

                // Keep in mind that we do not change the value send here, so in case the other side misses this Datagram
                // then we will send again when we get their next SEND_ME_THE_Datagram in response to seeing our I_HAVE_A_Datagram_4_U

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

        FOREACH_FACE(f) pending_Datagram_send_on_face[f]=true;        // This will trigger a send on all faces

    }

    // Deal with any Datagrams that came in to us

    FOREACH_FACE(f) {

        if ( isDatagramReadyOnFace( f ) )  {

            if ( getDatagramLengthOnFace(f) == sizeof ( colors ) ) {      // Just a double check to make sure the Datagram is the right length. Just stops corrupt Datagrams or Datagrams from other games from getting into our headspace.

                Color *receivedColors = (Color *) getDatagramOnFace( f ) ;

                FOREACH_FACE( g ) {

                    colors[g] = receivedColors[g];

                }

                updateDisplayColors();

            }

            // We are done with the datagram, so free up the buffer so we can get another
            markDatagramReadOnFace( f ); 


            pending_ack_send_on_face[f]=true;           // Tell other side that we got it so they can stop sending it


        }       // if ( isDatagramReadyOnFace( f ) )  {

    }            //  FOREACH_FACE(f) {

    // Now finally set the outgoing values on each face to reflect the Datagram state when nessisary

    FOREACH_FACE(f) {

        if (pending_ack_send_on_face[f]) {

            setValueSentOnFace( I_GOT_THE_DATAGRAM , f );

        } else {

            setValueSentOnFace( IDLE , f );

        }
    }       // FOREACH_FACE

}           // loop()
