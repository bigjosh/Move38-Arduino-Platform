// SuperColor Demo
// This is a HACK showing how to get superbright colors. 
// How to play:
// Push the button to make the color temporarily superbrght, double click to rotate through the colors
// green, red, and yellow. Note that you will only see yellow with a very string battery since otherwise 
// the red LED will lower the battery voltage so that the green can not fully turn on. 
// How it works:
// Normally a blink cycles though each of the 3 different LED colors (red,green,blue) on each pixel as it refreshes the display.
// This means that if a pixel is only RED, then it is off 2/3 of the time while the GREEN and BLUE values are being shown(1).
// In this demo, we disable the normal scanning function and then go straight to the hardware to  manually just turn on the the RED 
// LED sequentially on each pixel. This makes the color noticably brighter since the RED LED is on for a much higher precentage of the time. 

// (1) it is actually off 4/5th of the time since two times slots are needed to charge the chrge pump for the blue LED. 


#include "sfr_defs.h"
#include "iom168pb.h"

// RGB Sinks - We drive these cathodes low to light the selected color (note that BLUE has a charge pump on it)
//This will eventually be driven by timers

// Common Anodes - We drive these 1 to select Pixel.

#define PIXEL0_PORT PORTB
#define PIXEL0_DDR  DDRB
#define PIXEL0_BIT  6

#define PIXEL1_PORT PORTE
#define PIXEL1_DDR  DDRE
#define PIXEL1_BIT  0

#define PIXEL2_PORT PORTB
#define PIXEL2_DDR  DDRB
#define PIXEL2_BIT  7

#define PIXEL3_PORT PORTE
#define PIXEL3_DDR  DDRE
#define PIXEL3_BIT  1

#define PIXEL4_PORT PORTD
#define PIXEL4_DDR  DDRD
#define PIXEL4_BIT  4

#define PIXEL5_PORT PORTD
#define PIXEL5_DDR  DDRD
#define PIXEL5_BIT  2


#define LED_R_PORT PORTD
#define LED_R_DDR  DDRD
#define LED_R_BIT  6

#define LED_G_PORT PORTD
#define LED_G_DDR  DDRD
#define LED_G_BIT  5

#define LED_B_PORT PORTD
#define LED_B_DDR  DDRD
#define LED_B_BIT  3

#define TCCR0A  _SFR_IO8(0x24)

// Bit manipulation macros
#define SBI(x,b) (x|= (1<<b))           // Set bit in IO reg
#define CBI(x,b) (x&=~(1<<b))           // Clear bit in IO reg
#define TBI(x,b) (x&(1<<b))             // Test bit in IO reg

static void activateAnode( uint8_t led ) {

    switch (led) {

        case 0:
            SBI( PIXEL0_PORT , PIXEL0_BIT );
            break;

        case 1:
            SBI( PIXEL1_PORT , PIXEL1_BIT );
            break;

        case 2:
            SBI( PIXEL2_PORT , PIXEL2_BIT );
            break;

        case 3:
            SBI( PIXEL3_PORT , PIXEL3_BIT );
            break;

        case 4:
            SBI( PIXEL4_PORT , PIXEL4_BIT );
            break;

        case 5:
            SBI( PIXEL5_PORT , PIXEL5_BIT );
            break;

    }

}

// Deactivate all anodes. Faster to blindly do all of them than to figure out which is
// is currently on and just do that one.

static void deactivateAnodes(void) {

    // Each of these compiles to a single instruction
    CBI( PIXEL0_PORT , PIXEL0_BIT );
    CBI( PIXEL1_PORT , PIXEL1_BIT );
    CBI( PIXEL2_PORT , PIXEL2_BIT );
    CBI( PIXEL3_PORT , PIXEL3_BIT );
    CBI( PIXEL4_PORT , PIXEL4_BIT );
    CBI( PIXEL5_PORT , PIXEL5_BIT );

}

// Activate supercolor mode and enable red and/or green cathode as indicated

void superColorOn( bool colorGreenFlag , bool colorRedFlag ) {

  // Since the BIOS code cycles though pixel phases 0-5, we can stop it by manually setting the phase to 6
  // here. This makes the phase case statement fall though without touching any pixels. 
  blinkbios_pixel_block.phase = 6;

  // Turn off RED and GREEN hardware PWM timers.
   TCCR0A =  _BV( WGM00 ) | _BV( WGM01 );     // Red and green off timer and back to normal PORT control.  

  // Activate catchode(s) for the color(s) We want to show. 
  // Note that setting a cathode low makes the LED turn on. 
  if (colorGreenFlag) {
    CBI( LED_G_PORT , LED_G_BIT );
  } else {
    SBI( LED_G_PORT , LED_G_BIT );        
  }

  if (colorRedFlag) {
    CBI( LED_R_PORT , LED_R_BIT );
  } else {
    SBI( LED_R_PORT , LED_R_BIT );        
  }

  
}

// Go back to normal pixel display

void superColorOff() {

  // Turn hardware PWM timers back on. This takes over the PORT bit values. 
  TCCR0A =
      _BV( WGM00 ) | _BV( WGM01 ) |       // Set mode=3 (0b11)
      _BV( COM0A1) |                      // Clear OC0A on Compare Match, set OC0A at BOTTOM, (non-inverting mode) (clearing turns LED on)
      _BV( COM0B1)                        // Clear OC0B on Compare Match, set OC0B at BOTTOM, (non-inverting mode)
  ;

 // Resume normal BIOS scanning 
 blinkbios_pixel_block.phase = 0;

}

// Call repeatedly when supercolor is on to activate the next pixel

void superColorScan() {
  static byte p=0;

  p++;

  if (p==PIXEL_COUNT) {
    p=0;
  }

  deactivateAnodes();
  activateAnode( p );    
  
}

bool colorGreenFlag;
bool colorRedFlag;

void setup() {

  setColor(GREEN);
  colorGreenFlag=true;

}


void loop() {

  if (buttonPressed()) {
      superColorOn( colorGreenFlag , colorRedFlag );

      for(int i=0; i<2000;i++) {      // This sets how many times we scan this pixels
        superColorScan();
        for(byte j=0;j<200;j++) {     // This adds a little delay between pixels
          asm("nop");
        }
      }
      
      superColorOff();
  }

  if (buttonDoubleClicked()) {
    if (colorGreenFlag && colorRedFlag){
      setColor(GREEN);
      colorGreenFlag=true;            
      colorRedFlag=false;
    } else if ( colorGreenFlag ) {
      setColor(RED);
      colorGreenFlag=false;            
      colorRedFlag=true;
    } else {
      setColor(YELLOW);
      colorGreenFlag=true;            
      colorRedFlag=true;
    }
    
  }

}
    
