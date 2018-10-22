#include "blinkos.h"
#include "blinkos_button.h"
#include "button.h"

// BUTTON CONSTANTS

// Debounce button pressed this much
// Empirically determined. At 50ms, I can click twice fast enough
// that the second click it gets in the debounce. At 20ms, I do not
// any bounce even when I sllllooooowwwwly click.

#define BUTTON_DEBOUNCE_MS 20

// Delay for determining clicks
// So, a click or double click will not be registered until this timeout
// because we don't know yet if it is a single, double, or triple

#define BUTTON_CLICK_TIMEOUT_MS 330

#define BUTTON_LONGPRESS_TIME_MS 2000          // How long you must hold button down to register a long press.

// Click Semantics
// ===============
// All clicks happen inside a click window, as defined by BUTTON_CLICK_TIMEOUT_MS
// The window resets each time the debounced button state goes down.
// Any subsequent down events before the window expires are part of the same click cycle.
// If a cycle ends with the button up, then the clicks are tallied.
// If the cycle ends with the button down, then we interpret this that the user wanted to
// abort the clicks, so we discard the count.

static uint8_t debouncedButtonPosition=0;                     // Current debounced state

static uint8_t buttonDebounceCountdown=0;               // How long until we are done bouncing. Only touched in the callback
// Set to BUTTON_DEBOUNCE_MS every time we see a change, then we ignore everything
// until it gets to 0 again

static uint16_t clickWindowCountdown=0;                 // How long until we close the current click window. 0=done TODO: Make this 8bit by reducing scan rate.

static uint8_t clickPendingcount=0;                     // How many clicks so far int he current click window

static uint16_t longPressCountdown=0;                   // How long until the current press becomes a long press


// Called once per tick by the timer to check the button position
// and update the button state variables.

// Note: this runs in Callback context in the timercallback
// Called every 1ms

// Why do I hate pass by reference so bad? But I think we need it here so this function 
// can compile away into the single (timing sensitive) caller. 

uint8_t updateButtonState1ms(buttonstate_t &buttonstate) {
            
    uint8_t buttonChangeFlag=0;         // So we know what to return.

    uint8_t buttonPositon = button_down();

    if ( buttonPositon == debouncedButtonPosition ) {

        if (buttonDebounceCountdown) {

            buttonDebounceCountdown--;

        }

        if (longPressCountdown) {

            longPressCountdown--;

            if (longPressCountdown==0) {

                if (debouncedButtonPosition) {

                    buttonstate.bitflags|= BUTTON_BITFLAG_LONGPRESSED;
                    
                }
            }

            // We can nestle the click window countdown in here because a click will ALWAYS happen inside a long press...

            if (clickWindowCountdown) {

                clickWindowCountdown--;

                if (clickWindowCountdown==0) {      // Click window just expired

                    if (!debouncedButtonPosition) {              // Button is up, so register clicks

                        if (clickPendingcount==1) {                           
                            buttonstate.bitflags |= BUTTON_BITFLAG_SINGLECLICKED;                               
                        } else if (clickPendingcount==2) {
                           buttonstate.bitflags |= BUTTON_BITFLAG_DOUBECLICKED;
                        } else {
                            buttonstate.bitflags |= BUTTON_BITFLAG_MULITCLICKED;
                            buttonstate.clickcount = clickPendingcount;                                
                            // Note this could overwrite a previous multiclick count if more than 1 happens per loop cycle. 
                        }
                        
                        
                    }                            

                    clickPendingcount=0;        // Start next cycle (aborts any pending clicks if button was still down

                }

            } //  if (clickWindowCountdown)

        } //  if (longPressCountdown)


    }  else {       // New button position

        if (!buttonDebounceCountdown) {         // Done bouncing
            
            if (buttonPositon) {            // Button just pressed?
                
                buttonstate.bitflags |= BUTTON_BITFLAG_PRESSED;
                                               
                if (clickPendingcount<255) {        // Don't overflow
                    clickPendingcount++;
                }

                clickWindowCountdown = BUTTON_CLICK_TIMEOUT_MS ;
                longPressCountdown   = BUTTON_LONGPRESS_TIME_MS;

            } else {
                
                buttonstate.bitflags |= BUTTON_BITFLAG_RELEASED;

            }
            
            debouncedButtonPosition = buttonPositon;
            
            buttonChangeFlag =1 ;
            
        }

        // Restart the countdown anytime the button position changes

        buttonDebounceCountdown = BUTTON_DEBOUNCE_MS;

    }

    buttonstate.down = debouncedButtonPosition;
    
    return buttonChangeFlag;
}

