
// Here are our magic shared memory links to the BlinkBIOS running up in the bootloader area.
// These special sections are defined in a special linker script to make sure that the addresses 
// are the same on both the foreground (this blinklib program) and the background (the BlinkBIOS project compiled to a HEX file)

#include "blinkbios_shared_button.h"
#include "blinkbios_shared_millis.h"
#include "blinkbios_shared_pixel.h"
#include "blinkbios_shared_irdata.h"
#include "blinkbios_shared_slack.h"

// Here are the actual allocations for the shared memory blocks

// We put each in its own section so that the separately compiled blinkos will be able to find them.

blinkbios_pixelblock_t      __attribute__ ((section (".ipcram1")))    blinkbios_pixel_block;
blinkbios_millis_block_t    __attribute__ ((section (".ipcram2")))    blinkbios_millis_block;
blinkbios_button_block_t    __attribute__ ((section (".ipcram3")))    blinkbios_button_block;
blinkbios_irdata_block_t    __attribute__ ((section (".ipcram4")))    blinkbios_irdata_block;
blinkbios_slack_block_t     __attribute__ ((section (".ipcram5")))    blinkbios_slack_block;

// Here is our entry point. We are called by the BlinkBIOS after everything is set up and ready
// Note that this is not a normal startup, we are staring right from flash address 0x000 with no
// vector table at all. We don't need one because all vectors are pointing to the BlinkBIOS
// running up in the bootloader area. 

// We will never return, so don't need any of the extra formality, just give me the straight up code

int main(void) __attribute__ ((section (".init9"))) __attribute__((used)) __attribute__ ((naked));

int main(void) {
        
    run(); 
        
}   