// Take a look at the instant button position and update the button state flags
// in loopstate

// Depends on being called once per ms or timing constants will not work right

// Returns 1 if the button changed position (debounced). Useful for reseting sleep timer.

uint8_t updateButtonState1ms(buttonstate_t &buttonstate);
    