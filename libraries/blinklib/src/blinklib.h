/*
 * blkinklib.h
 *
 * This defines a statefull view of the blinks tile interactions with neighbors.
 *
 * In this view, each tile has a "state" that is represented by a number between 1 and 127.
 * This state value is continuously broadcast on all of its faces.
 * Each tile also remembers the most recently received state value from he neighbor on each of its faces.
 *
 * Note that this library depends on the blinklib library for communications with neighbors. The blinklib
 * IR read functions are not available when using the blinkstate library.
 *
 * Note that the beacon transmissions only occur when the loop() function returns, so it is important
 * that sketches using this model return from loop() frequently.
 *
 */

#ifndef BLINKLIB_H_
#define BLINKLIB_H_

#include "pixelcolor.h"

// The value of the data sent and received on faces via IR can be between 0 and IR_DATA_VALUE_MAX
// If you try to send higher than this, the max value will be sent.

#define IR_DATA_VALUE_MAX 63

// Returns the last received value on the indicated face
// Between 0 and IR_DATA_VALUE_MAX inclusive
// returns 0 if no neighbor ever seen on this face since power-up
// so best to only use after checking if face is not expired first.

byte getLastValueReceivedOnFace( byte face );

// Did the neighborState value on this face change since the
// last time we checked?

// Note the a face expiring has no effect on the last value

byte didValueOnFaceChange( byte face );

// false if messages have been recently received on the indicated face
// (currently configured to 100ms timeout in `expireDurration_ms` )

byte isValueReceivedOnFaceExpired( byte face );

// Returns false if their has been a neighbor seen recently on any face, returns true otherwise.
bool isAlone();

// Set value that will be continuously broadcast on specified face.
// Value should be between 0 and IR_DATA_VALUE_MAX inclusive.
// If a value greater than IR_DATA_VALUE_MAX is specified, IR_DATA_VALUE_MAX will be sent.
// By default we power up with all faces sending the value 0.

void setValueSentOnFace( byte value , byte face );

// Same as setValueSentOnFace(), but sets all faces in one call.

void setValueSentOnAllFaces( byte value );

/* --- Long packet processing */

// Note that long packets have a 1 byte checksum added by blinklib. Packets with the wrong checksum are tossed.

#define IR_LONG_PACKET_MAX_LEN 32

// Returns the number of bytes waiting in the data buffer, or 0 if no packet ready.
byte getPacketLengthOnFace( uint8_t face );

// Returns true if a packet is available in the buffer
boolean isPacketReadyOnFace( uint8_t face );

 // Returns a pointer to the actual received data
 // This should really be a (void *) so it can be assigned to any pointer type,
 // but in C++ you can not case a (void *) into something else so it doesn't really work there
 // and I think too ugly to have these functions that are inverses of each other to take/return different types.
 // Thanks, Stroustrup.
const byte *getPacketDataOnFace( uint8_t face );

// Frees up the buffer holding the long packet. This is not necessary because the
// buffer will be automatically released when loop() returns, but it is good
// practice to manually release it as soon as you are done with to to make room
// for new incoming data.
void markLongPacketRead( uint8_t face );

// Send a long data packet.  Packet is protected by checksum.
// Return 1 if sent.
// Returns 0 if not able to send because (1) there is currently data being received on that face, (2) the len exceeds IR_LONG_PACKET_MAX_LEN

uint8_t sendPacketOnFace( byte face , const byte *data, byte len );


/*

	This set of functions let you test for changes in the environment.

*/

/*

    Button functions

*/

// Debounced view of button state. true if the button currently pressed.

bool buttonDown(void);

// Was the button pressed or lifted since the last time we checked?
// Note that these register the change the instant the button state changes
// without any delay, so good for latency sensitive cases.
// It is debounced, so the button must have been in the previous state a minimum
// debounce time before a new detection will occur.

bool buttonPressed(void);
bool buttonReleased(void);

// Was the button single, double , or multi clicked since we last checked?
// Note that there is a delay after the button is first pressed
// before a click is registered because we have to wait to
// see if another button press is coming.
// A multiclick is 3 or more clicks

// Remember that these click events fire a short time after the button is lifted on the final click
// If the button is held down too long on the last click, then click interaction is aborted.

bool buttonSingleClicked();

bool buttonDoubleClicked();

bool buttonMultiClicked();


// The number of clicks in the longest consecutive valid click cycle since the last time called.
byte buttonClickCount(void);

// Remember that a long press fires while the button is still down
bool buttonLongPressed(void);

/*

	This set of functions lets you control the colors on the face RGB LEDs

*/

// Set our state to newState. This state is repeatedly broadcast to any
// neighboring tiles.
// Note that setting our state to 0 make us stop broadcasting and effectively
// disappear from the view of neighboring tiles.

// TODO: State view not implemented yet. You can use irBroadcastData() instead.

// void setState( byte newState );

// Color type holds 4 bits for each R,G,B. Top bit is currently unused.

// TODO: Do we need 5 bits of resolution for each color?
// TODO: Use top bit(s) for something useful like automatic
//       blink or twinkle or something like that.

// Argh, these macros are so ugly... but so ideomatic arduino. Maybe use a class with bitfields like
// we did in pixel.cpp just so we can sleep at night?

typedef pixelColor_t Color;

// Number of brightness levels in each channel of a color
#define BRIGHTNESS_LEVELS_5BIT 32
#define MAX_BRIGHTNESS_5BIT    (BRIGHTNESS_LEVELS_5BIT-1)

#define GET_5BIT_R(color) (color.r)
#define GET_5BIT_G(color) (color.g)
#define GET_5BIT_B(color) (color.b)

// R,G,B are all in the domain 0-31
// Here we expose the internal color representation, but it is worth it
// to get the performance and size benefits of static compilation
// Shame no way to do this right in C/C++

#define MAKECOLOR_5BIT_RGB(r,g,b) (pixelColor_t(r,g,b,1))

#define RED         MAKECOLOR_5BIT_RGB(MAX_BRIGHTNESS_5BIT, 0                    ,0)
#define ORANGE      MAKECOLOR_5BIT_RGB(MAX_BRIGHTNESS_5BIT,MAX_BRIGHTNESS_5BIT/2 ,0)
#define YELLOW      MAKECOLOR_5BIT_RGB(MAX_BRIGHTNESS_5BIT,MAX_BRIGHTNESS_5BIT   ,0)
#define GREEN       MAKECOLOR_5BIT_RGB( 0                 ,MAX_BRIGHTNESS_5BIT   ,0)
#define CYAN        MAKECOLOR_5BIT_RGB( 0                 ,MAX_BRIGHTNESS_5BIT   ,MAX_BRIGHTNESS_5BIT)
#define BLUE        MAKECOLOR_5BIT_RGB( 0                 , 0                    ,MAX_BRIGHTNESS_5BIT)
#define MAGENTA     MAKECOLOR_5BIT_RGB(MAX_BRIGHTNESS_5BIT, 0                    ,MAX_BRIGHTNESS_5BIT)

#define WHITE       MAKECOLOR_5BIT_RGB(MAX_BRIGHTNESS_5BIT,MAX_BRIGHTNESS_5BIT   ,MAX_BRIGHTNESS_5BIT)

#define OFF         MAKECOLOR_5BIT_RGB( 0                 , 0                    , 0)

// This maps 0-255 values to 0-31 values with the special case that 0 (in 0-255) is the only value that maps to 0 (in 0-31)
// This leads to some slight non-linearity since there are not a uniform integral number of 1-255 values
// to map to each of the 1-31 values.

// Make a new color from RGB values. Each value can be 0-255.

Color makeColorRGB( byte red, byte green, byte blue );

#define MAX_BRIGHTNESS (255)

// Dim the specified color. Brightness is 0-255 (0=off, 255=don't dim at all-keep original color)
// Inlined to allow static simplification at compile time

inline Color dim( Color color, byte brightness) {
    return MAKECOLOR_5BIT_RGB(
        (GET_5BIT_R(color)*brightness)/255,
        (GET_5BIT_G(color)*brightness)/255,
        (GET_5BIT_B(color)*brightness)/255
    );
}

// Make a new color in the HSB colorspace. All values are 0-255.

Color makeColorHSB( byte hue, byte saturation, byte brightness );

// Change the tile to the specified color
// NOTE: all color changes are double buffered
// and the display is updated when loop() returns

void setColor( Color newColor);

// Set the pixel on the specified face (0-5) to the specified color
// NOTE: all color changes are double buffered
// and the display is updated when loop() returns

void setColorOnFace( Color newColor , byte face );

// DEPREICATED: Use setColorOnFace()
void setFaceColor( byte face , Color newColor ) __attribute__ ((deprecated));
void setFaceColor(  byte face, Color newColor );


/*

    Timing functions

*/

// Number of running milliseconds since power up.
//
// Important notes:
// 1) does not increment while sleeping
// 2) is only updated between loop() interations
// 3) is not monotonic, so always use greater than
//    and less than rather than equals for comparisons
// 4) overflows after about 50 days
// 5) is only accurate to about +/-10%

unsigned long millis(void);

#define NEVER ( (uint32_t)-1 )          // UINT32_MAX would be correct here, but generates a Symbol Not Found.

class Timer {

	private:

		uint32_t m_expireTime;		// When this timer will expire

	public:

		Timer() : m_expireTime(0) {};		// Timers come into this world pre-expired.

		bool isExpired();

        uint32_t getRemaining();

        void set( uint32_t ms );            // This time will expire ms milliseconds from now

		void add( uint16_t ms );

        void never(void);                   // Make this timer never expire (unless set())

};


/*

    Utility functions

*/


// Return a random number between 0 and limit inclusive.

uint16_t random( word limit );

// Read the unique serial number for this blink tile
// There are 9 bytes in all, so n can be 0-8

byte getSerialNumberByte( byte n );

// Map one set to another

long map(word x, word in_min, word in_max, word out_min, word out_max);



/*

    IR communications functions

*/


// Broadcast data on all faces. Data is 6-bits wide (0-63 value).

void irBroadcastData( uint8_t data );

// Is there a received data ready to be read on the indicated face? Returns 0 if none.

bool irIsReadyOnFace( uint8_t face );

// Read the most recently received data. Blocks if no data ready. Data is 6-bits wide (0-63 value).

uint8_t irGetData( uint8_t face );


/* Power functions */

// The blink will automatically sleep if the button has not been pressed in
// more than 10 minutes. The sleep is preemptive - the tile stops in the middle of whatever it
// happens to be doing.

// The tile wakes from sleep when the button is pressed. Upon waking, it picks up from exactly
// where it left off. It is up to the programmer to check to see if the blink has slept and then
// woken and react accordingly if desired.


// Returns 1 if we have slept and woken since last time we checked
// Best to check as last test at the end of loop() so you can
// avoid intermediate display upon waking.

uint8_t hasWoken(void);

/*

    These hook functions are filled in by the sketch

*/


// Called when this sketch is first loaded and then
// every time the tile wakes from sleep

void setup(void);

// Called repeatedly just after the display pixels
// on the tile face are updated

void loop();


/*

	Some syntactic sugar to make our programs look not so ugly.

*/


#define FACE_COUNT 6

// 'Cause C ain't got iterators and all those FOR loops are too ugly.
#define FOREACH_FACE(x) for(int x = 0; x < FACE_COUNT ; ++ x)       // Pretend this is a real language with iterators

// Get the number of elements in an array.
#define COUNT_OF(x) ((sizeof(x)/sizeof(x[0])))


#endif /* BLINKLIB_H_ */
