#include <avr/eeprom.h>

#include <avr/pgmspace.h>

#include <util/crc16.h>

#include "blinklib.h"
#include "shared/blinkbios_shared_functions.h"

#include "gamestat.h"

uint8_t saveGameStat( const void *gamestatData  , uint8_t gameStatLen ) {
	
	// Check if we are even running on an NFC blink. 
	if (!isNFCblink()) {
		return 2;			// Not an NFC blink failure
	}
		
	return BLINKBIOS_MULITPLEX_VECTOR( BLINKBIOS_MULITPLEX_FUNCTION_SAVE_GAMESTAT , gamestatData , gameStatLen );
		
}
