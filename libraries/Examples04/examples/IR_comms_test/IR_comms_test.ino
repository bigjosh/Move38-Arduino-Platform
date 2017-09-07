/*  Blinks Send/Receive (Test)
 *   
 *  Test the IR transmitt and recieve on the Blinks faces 
 *  
 *  At startup, the tile in in recieve mode waiting for data on all faces.
 *  When it recieves a color on a face, it will display that color on the face for 0.1 seconds.
 *  
 *  Pressing the button switches to transmit mode where the blink transmits the currently displayed
 *  color on all faces at about 10Hz. Button presses cycle though transmitting red, green, blue, and yellow... 
 *  and finally back to receive mode.
 *     
 *  by Jonathan Bobrow
 *  www.Move38.com
 *  08.29.2017
 *  
 *  Updated by josh 9/7/17
 */


static const uint8_t colors[][3] = {
     {200,0,0},         // Red
     {0,200,0},         // Green
     {0,0,200}          // Blue
};


// https://stackoverflow.com/questions/4415524/common-array-length-macro-for-c
#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

static_assert( COUNT_OF(colors) <= 4 , "We can only send dibits for now, so max number of send states (therefore colors) is 4!" );

bool isSending = false; 
uint8_t sendColor =0;     // Color we are currently sending if in send mode

void setup() {
  
  // animate red circle to show reset or startup
  FOREACH_FACE( i ) {  
        setPixelRGB(i,200,0,0);
        delay(100);
        setAllRGB(0,0,0);        // Show pixels one at a time, and leave display clear when circle finished
  }
}

uint8_t rx_blink_color[FACE_COUNT][3];  // Remember current color so we can decay it 

void loop() {

    // switch through 4 modes ( 0 = receive, 1,2,3 = sending 1,2,or 3 )
    
    if( buttonPressed() ) {       // Note that we will get some occasional bouncing here, usually on the way back up. 
                                  // Easily fixed with a higher level button interface... coming soon!
  
      if ( isSending ) {

        sendColor++;

        if ( sendColor >= COUNT_OF( colors ) ) {   // Did we cycle though all colors?

          isSending = false;                        // If so, go back to receive mode

          // Now in display mode. Clear out any leftover colors from last time.
          // Not stricktly nessisary, but neatness counts.

          FOREACH_FACE( i ) {
            rx_blink_color[i][0] = 0;
            rx_blink_color[i][1] = 0;
            rx_blink_color[i][2] = 0;          
          }
        }

      } else {

        isSending = true;
        sendColor=0;
        
      }            
      
    }

    
    if (isSending) {

      // Blink the color we are about to send on all faces to show we are sending...

      setAllRGB(                   
        colors[sendColor][0],
        colors[sendColor][1],
        colors[sendColor][2]
      );

      irSendAllDibit( sendColor );    // Scream it from the rooftops!

      delay(10);     // Make the color stay on the pixels long enough to actually see it        
      setAllRGB( 0 ,0 , 0 );    

      delay(90);    // slow down to about 10Hz so the slow humans can actually see it on both TX and RX side

    } else { // Not isSending, so in recieve mode...

      FOREACH_FACE( i ) {

        if(irIsAvailable(i)) {      // Any data recieved?

          // get value read on face
          int val = irReadDibit(i);

          // Start displaying the recieved color full blast

          // We really need a less verbose color model here!
          rx_blink_color[i][0] = colors[val][0];
          rx_blink_color[i][1] = colors[val][1];
          rx_blink_color[i][2] = colors[val][2];

        } else {

          // Exponetial decay of curently displayed colors happens to look nice, and automatically stops when we get to 0
        
          rx_blink_color[i][0] = rx_blink_color[i][0]/2;
          rx_blink_color[i][1] = rx_blink_color[i][1]/2;
          rx_blink_color[i][2] = rx_blink_color[i][2]/2;


        }

        // now display whatever the current face color is...
          
        setPixelRGB( i,
          rx_blink_color[i][0],
          rx_blink_color[i][1],
          rx_blink_color[i][2]

        );
          
      }
      
    }
    
}


