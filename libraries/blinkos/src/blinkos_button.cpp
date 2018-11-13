#include "blinkos.h"
#include "blinkos_button.h"
#include "button.h"
#include "bootloader.h"

// BUTTON CONSTANTS

// Debounce button pressed this much
// Empirically determined. At 50ms, I can click twice fast enough
// that the second click it gets in the debounce. At 20ms, I do not
// any bounce even when I sllllooooowwwwly click.

#define BUTTON_DEBOUNCE_MS 20

// Delay for determining clicks
// So, a click or double click will not be registered until this timeout
// because we don't know yet if it is a single, double, or triple

#define BUTTON_CLICK_TIMEOUT_MS      330

#define BUTTON_LONGPRESS_TIME_MS    2000          // How long you must hold button down to register a long press.
#define BUTTON_SEEDPRESS_TIME_MS    6000
#define BUTTON_OFFPRESS_TIME_MS     7000

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

static uint8_t longpressRegisteredFlag=0;               // Did we register a long press since the button went down?
                                                        // Avoids multiple long press events from the same down event
                                                        // We could keep an independant longpressCountup, but that would be a 16 bit so heavy to increment and compare.

static uint16_t pressCountup=0;                         // Start counting up when the button goes down to detect long presses

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

        // Debounced button position has not changed

        if (buttonDebounceCountdown) {

            buttonDebounceCountdown--;

        }

        if (buttonPositon) {        // Button currently down?

            if (clickWindowCountdown) {             // Was the button pressed less than a click window ago?

                clickWindowCountdown--;

                if (clickWindowCountdown==0) {      // Click window just expired (only catches the transition from not-expired to expired while button is up)

                    clickPendingcount=0;            // We are still down, so toss any pending clicks

                }

            }

            pressCountup++;          // Don't need to worry about overflow because we will seed or turn off before then.

            if (pressCountup > BUTTON_OFFPRESS_TIME_MS) {

                // Force off

            } else if (pressCountup > BUTTON_SEEDPRESS_TIME_MS ) {

                // This will never return

                JUMP_TO_BOOTLOADER_SEED();


            } else if ( pressCountup > BUTTON_LONGPRESS_TIME_MS ) {

                if (!longpressRegisteredFlag) {

                    buttonstate.bitflags|= BUTTON_BITFLAG_LONGPRESSED;

                    longpressRegisteredFlag=1;          // Don't register this one again until a button up event

                }


            }



        } else {  //  if (!buttonPositon) {  -- button currently up

            if (clickWindowCountdown) {             // Was the button pressed less than a click window ago?

                clickWindowCountdown--;

                if (clickWindowCountdown==0) {      // Click window just expired (only catches the transition from not-expired to expired while button is up)

                    // Button is up, so register any clicks that happened inside this window

                    if (clickPendingcount==1) {
                        buttonstate.bitflags |= BUTTON_BITFLAG_SINGLECLICKED;
                    } else if (clickPendingcount==2) {
                        buttonstate.bitflags |= BUTTON_BITFLAG_DOUBECLICKED;
                    } else {
                        buttonstate.bitflags |= BUTTON_BITFLAG_MULITCLICKED;
                        buttonstate.clickcount = clickPendingcount;
                        // Note this could overwrite a previous multiclick count if more than 1 happens per loop cycle.
                    }

                    clickPendingcount=0;        // Start next cycle (aborts any pending clicks if button was still down

                }


            } //  if (clickWindowCountdown)

        }  // if (buttonPositon) {        // Button currently down?


    }  else {       // New button position

        if (buttonDebounceCountdown==0) {         // Done bouncing

            if (buttonPositon) {

                // Button down event

                buttonstate.bitflags |= BUTTON_BITFLAG_PRESSED;

                if (clickPendingcount<255) {        // Don't overflow
                    clickPendingcount++;
                }

                clickWindowCountdown = BUTTON_CLICK_TIMEOUT_MS ;
                pressCountup         = 0;

            } else {

                // Button up event

                buttonstate.bitflags |= BUTTON_BITFLAG_RELEASED;

                if (clickWindowCountdown==0) {      // Button down longer than the click window?

                    clickPendingcount=0;            // Don't register any clicks.
                                                    // Clickwindow stays at 0 until next down event

                }

                longpressRegisteredFlag = 0;        // Now that the button is released, we can register another long press on the next down.

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

