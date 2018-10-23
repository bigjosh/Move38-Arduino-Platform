# Bootloader Concepts

A typical blink will have three different programs in flash memory...

1. Bootloader 
2. Built-in game
3. Active game

The active game is the one currently running. It can either be the same as the built-in game, or it could have been loaded from another blink. 

The bootloader is responsible for loading the active game either by coping the built-in game or downloading over the IR links from another tile. 

The bootloader runs the `blinkos` code and handles any functions that could not reasonably be handled by application code, including...

1. Start up initialization.
1. Loading the active game either by coping the built-in game or downloading a new game from the IR links.
2. Refreshing the pixels.
3. Monitoring for button presses.
4. Handling low level IR communications.
5. Monitoring battery level. 
6. Sleeping and waking.
 
You can find more information about the API that `blinkos` code exposes to the application here...

(add blinkos link)  

The bootloader lives in the AVR's boot loader area at the top of flash memory. The

## Power up


We keep the code that loads new applications over IR in the bootloader section. 

We set the bootloader to its smallest size so that potentially we might be able to reprogram some of the `blinkos` code over the IR link (only code in the bootloader section can do flash programming easily). The smallest bootloader size on the ATMEGA168PB is 2Kb, and this is selected by the default fuse settings.  

Since the `BOOTRST` fuse is programmed, so the AVR immediately jumps to the boot reset address, which is `0x1C00`. This address has a jump to the beginning of the bootloader, which comes right after the ISR table (which itself follows at `0x1C02`).

The bootloader sets `IVSEL`, which activates the bootloader's ISR table rather than the one at the bottom of flash memory where the active game will live. This is important so that the bootloader can use these interrupts to potentially load a new game. Note that once an active game is loaded ans started, then that game could potentially clear the `IVSEL` bit again to take over interrupts if it really wanted to. (Pretty crappy from a security point of view).    
 
The the bootloader shows the power-up animation and spends the next 1 second looking for a "game push" command on one of its faces. 

If a "game push" is seen, then the blink starts loading the new game into the active area from the face where it saw the push. 

If a "game push" is not seen, then the blink copies the built-in game into the active area and then jumps to it. 

## Starting a game

The `blinkos` starts the active game by jumping to `0x000`.

