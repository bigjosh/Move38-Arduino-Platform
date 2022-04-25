// Save a game statistic to be uploaded next time this blink connects to a phone via NFC.
// The internal format of the stat is up to the game. Whatever you put here will go up to the game stats server
// and it will decode it. The gamestat can be up to BLINKBOIS_GAMESTAT_DATA_MAXLEN bytes long. 
//
// Your game must be registered with the good folks at Move38.com for the game stats to actually show up anywhere. 
// This call will be ignored in non-NFC blinks. 
//
// Returns:
// 0 on success
// 1 on failure because passed len greater than max allowed len 
// 2 on failure becuase not an NFC blink

uint8_t saveGameStat( const void *gamestatData , uint8_t gameStatLen );
