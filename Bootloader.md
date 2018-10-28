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

## IR game load sequence

A blink initiates sending a game by transmitting a IR_PACKET_HEADER_PULLRERQUEST packet. For error detection, the 2nd byte of this packet contains the same bits as the header just inverted. 

When you get a pull request, you should reboot which will cause the bootloader to run.

The bootloader starts listing on the IR LEDs and if it hears a pull, request then it goes into download mode.

In download mode, you repeatedly request blocks using the IR_PACKET_HEADER_PULLFLASH. The 2nd byte indicates the requested block. No error check is needed here since if the sending tile happens to miss the request because of a corruption then we will time out and ask again. If it happens to send the wrong block, that is ok too since the block number is protected by CRC in the push packet, so if we get the wrong block we can still use it. 

There are 2 timeouts. If we time out waiting for a block after a pull, in which case we request the block again. There is also a longer timeout that if we don't see any blocks that get us closer to our goal (or some other metric that things are not progessing), then the bootloader copies the built-in game from upper flash down to the active area starting at 0x0000. 

Once an active applicatiuon is loaded (either by completing the IR, or from coping the built-in one), then the bootloader clears `IVSEL` so that the interrupt vector is back to the application, and then jumps to the application reset vector at 0x000. 


The active game flash area is

## Flash layout

We need as big a bootloader as we can get, so we set the bootloader to fill the entire NRWW flash area. Note that only code running in this area can run while flash is being written to , so we really want to fit the whole bootloader up here so we don't have to worry about parts of is getting touch while programming.    

The bootloader the starts at flash address 0x1C00 by setting the fuse BOOTSZ=00. This gives us 1024 words (2048 bytes) of flash for the bootloader. 

At the very bottom of the bootloader is the reset vector, which points to the start of the bootload code. We set the BOOTRST=0 fuse to make the chip jump to this vector on reset (rather than starting the application code reset vector at 0x000).   

The bootloader sets `IVSEL`, which activates the bootloader's ISR table rather than the one at the bottom of flash memory where the active game will live. This is important so that the bootloader can use these interrupts.

Note that the bootloader does not need all the ISR vectors, so it can use a shortened table to get a few more bytes of code to work with.   

The actual program memory that holds the games starts at flash address 0x000. There is a total of 16K bytes of flash on the ATMEGA168PB, which is organized into 128 pages of 64 words (128 bytes) each. 

This flash space is shared by...

| What | Starts | Ends | Length (Bytes) |
| - | -:| -:| -:|
| Active game | 0x0000 | 0x0dff | 7Kb | 
| Built-in game | 0x0e00 | 0x1bff | 7Kb | 
| Bootloader | 0x1c00 | 0x1fff | 2Kb |

(Sometimes flash memory addresses are given in words rather than bytes. The addresses above are bytes)

The active game is the game that will actually get executed. The built-in game must be copied down to the active game area before it can be played (in which was there are two copied of the built-in game in flash). You can't 

Note that the built-in game must be copied into the active 

The active game starts at at 0x0000 and can be up to 7Kb long. Each game is self sufficient and can be started without the bootloader present (this helps with development).


To get the bootloader up where it belongs in memory, we have add a new section called `bls` with an address of `0x1c00` to the linker. The GCC linker doubles this word address to make the byte address of `0x3400`, which is the start of the boot loader section with the `BOOTSZ` bit set to `00` (the max bootloader size). 

### Start-up tricks

To have as much room as possible for the bootloader, we do some pretty drastic tricks. We completely remove the normal C runtime startup code and replace it with a single instructions to clear the zero register (`r1`). We do not need to set up the stack because [the chip does it automatically](https://ognite.wordpress.com/2013/11/14/true-chaff-superfluous-stack-pointer-and-status-register-setting/). We also declare the main function to be `naked` to suppress the normal stack setup and return, and then we put it in the `init9` section which comes right after all the code that sets up the variables, so there is no `call` (or even `jmp`!) into main - the code just falls though to it. 

### Bootloader fuses  

To get this large 2k bootloader area, the `BOOTZ` fuse bits must be programmed to `00` (this is the factory default).   

At startup, the chip starts executing the bootloader reset vector `0x1c00`. The requires the `BOOTRST` fuse to be programmed to `0` (not the default). IF the bootloader decides that it does not need to load anything (or after it is done loading) then it jumps to `0x000` to start the application code. 

## Packet formats

### IR_PACKET_HEADER_PULLREQUEST
    
| `0b01101010` | length in blocks | program checksum  low byte | program checksum high byte | 

This is transmitted by a tile that is in game sending mode. 

When we get this, we reboot into the bootloader. If the bootloader sees this before the time out then it will go into game download mode.

The checksum is a simple mod 0xffff checksum of the full game. We use this rather than a CRC becuase the checksum works even if blocks are received out of order.


 ### IR_PACKET_HEADER_PULLFLASH       

| `0b01011101` | requested block |

Once we are in game download mode, we repeatedly ask out neighbors for new blocks using this packet. We only ask neighbors who we've seen a pull request from that matches the checksum we are downloading. 

### IR_PACKET_HEADER_PUSHFLASH

| 0b01011101 |  game checksum low byte | game checksum high byte | block number | 128 bytes of flash page | block checksum byte |   

A game in sending mode will reply to a pull packet with a push packet. The game checksum is included so the receipt can verify it is for the correct game. The total length of this packet is 133 bytes.  

Since FLASH pages are 128 bytes long on this chip and we send a full page in each packet, it will ideally take [56 push packets](https://www.google.com/search?rlz=1C1CYCW_enUS687US687&ei=FpzQW_7XMuuMggevqb_YDw&q=%28%280x0e00*2%29+%2F+128+%29+in+decmial&oq=%28%280x0e00*2%29+%2F+128+%29+in+decmial&gs_l=psy-ab.3...9892.17114..17387...0.0..0.67.544.9......0....1..gws-wiz.......0i71j33i10.rIT_Mh01HY4) to send a full game (assuming no errors or dropped packets).  


## Compiling

We need all the space we can get so we pass gcc the `-Os` and `-flto`  and we pass the linker `-fwhole-program`. This gets us about 20 extra bytes.

You can read about Link Time Optimization here...
https://github.com/arduino/Arduino/issues/660

(NB This does not seem to work. Maybe we need to copy all the core files into the blinkboot directory and compile them in one shot rather than having an intermediate lib file for the core?)