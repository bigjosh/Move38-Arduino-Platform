# Bootloader Concepts

A typical blink will have three different programs in flash memory...

1. Bootloader 
2. Built-in game
3. Active game

The active game is the one currently running. It can either be the same as the built-in game, or it could have been loaded from another blink. 

The bootloader is responsible for loading the active game either by copying the built-in game or downloading over the IR links from another blink.

The only way the built-in game can run is after being copied down to the active area first. This is because all the addresses in every game are referenced from the 0 base address by the linker. Copying the built-in game to the active area first also makes it so that the same exact image is running regardless of what the source (local or downloaded) of the active game was.

The bootloader lives in the AVR's special bootloader area at the top of flash memory. This memory is special because the hardware will only allow code in this area to write to flash memory. (There are ways around this, but not really practical for this application.) 

The maximum bootloader size possible on this chip is 2KB. This leaves 14KB for the games, which is split up 7KB for the active game and 7KB for the built-in game image. 

The bootloader and built-in game image are typically programmed at the factory and must be present. Right now the game image is total independent, so it is possible to program a blink with a game directly and have it just run (mostly useful for development), but it is expected that we will move some functions (like pixel updates, IR updating, and maybe button updating) up to the bootloader to avoid having them duplicated in each game image.  

## Bootloader state diagram

!(Bootloader state diagram)[bootloader.svg]


## Power up

The `BOOTRST` fuse is programmed, so the AVR immediately jumps to the bootloader reset address, which is at the beginning of the bootloader memory area at byte address `0x3800`. This address has a jump to the beginning of the bootloader, which comes right after the ISR table (which itself follows at `0x3804`). We get the bootloader to start here by defining the `.text` section to start at `0x1c00` to the linker. Note that `0x1c00` is half of `0x3800` because the linker doubles the address to convert it from a word address to a byte address.  

The bootloader sets `IVSEL`, which activates the bootloader's ISR table rather than the one at the bottom of flash memory where the active game will live. This is important so that the bootloader can use these interrupts while programming a new game into the active area. Note that once an active game is loaded and started, then that game could potentially clear the `IVSEL` bit again to take over interrupts if it wants to. Currently games do this so they are self-contained, but soon hopefully most games will want to leave the bootloader ISRs in place to save space. A game could also potentially take over only some of the ISRs by setting the vectors it wants to replace in the lower ISR table and using jumps up to the ones in the bootloader for the others, and then clearing `IVSEL`.     

At bootup, bootloader the blink copies the built-in game into the active area and then jumps to it. This gives an escape hatch so that reseting or pulling the battery on a blink always results in a good active image. Otherwise a downloaded active image could disable the bootloader's ability to run and lock it self in.  

## Starting a game

The bootloader starts the active game by jumping to `0x000`. The game can assume....

1. interrupts are on
2. interrupts are pointing to the bootloader table
3. the `zero_reg` has been set to `0`
4. the stack pointer is at the top of the stack
5. the clock speed, pixels, and IR have been initialized and are enabled

Not having to do these saves some code space in the game and let's it get right down to business on launch. If the game leaves the ISR table pointing to the bootloader it could even potentially start executing real code right at the reset vector address and reclaim all of the flash space that would normally be consumed buy the table jumps.    

## IR game load sequence

To initiate a game transfer, the user holds the button for 3 seconds. When the bootloader sees this, it waits for the button to go up and then copies the built-in game to the active area, and (2) goes into Seeding mode below. If the button is not released for 3 seconds (total of 6 seconds) then the blink goes to sleep.

If a blink ever sees a Seeking packet on any face and the checksum is different than the checksum for the game currently in the active area, then it goes into download mode and starts downloading from the face where it got the Seeding packet. In download mode the blink repeatedly sends Pull packets requesting the next page of flash data. If it does not get an answer within 30ms, then it resends the Pull. If the download completes successfully, the blink itself goes into Seeding mode. If the download times out (100ms), then the blink reboots, loading the built-in game and jumping to it. This ensures that there is always a valid image in the active area. Once the built-in game is running, it is possible that it will get another Seed packet that will restart the load sequence- hopefully with better luck.   

If a blink receives a Seeding packet that matches its own checksum, then it goes into Seeding mode.

In Seeding mode there are two countdown timers, Face timeout (100ms) and Seed mode timeout (1s). On entering Seed mode, the Face timeout is set to zero and the Seed mode timeout is reset to 1s.

Note that the Face timeout is >3x the Pull timeout. This gives a blink that is pulling 3 retries to get a good push from a seeding tile before that tile moves on to sending a Seed packet to the next adjacent blink. Note that in this case, the pulling blink will load the built-in game and start it. Then when the Seeding blink gets back around and sends it a new Seed packet it will restart the process and hopefully have more luck.   

Whenever the Face timeout expires (it starts expired on entering Seed mode too), the blink advances to the next face and sends a Seed packet and resets the Face timeout.

Whenever the Seed mode timer expires, the blink jumps to the active area. (The active game could either be recently downloaded or the built-in one depending on how we got here.)        

Note that the net effect of this is that each blink will stay in Seeding mode as long as it is connected by any path to a blink that has not yet downloaded the new game.

When the blink that originally started the Seed (by manual long button press) finally itself times out of Seed mode, then know that all connected blinks have loaded the new game. A game can   
  
In Seeding mode, the blink sends out Seeding packets on all faces. These packets contain the checksum of the game currently in the active area. Any time it gets back a Pull packet on a face, it replies with a Push packet with flash data for the currently active game.

If the blink receives a Seeding packet while it is in active Seeding mode itself, it looks at the checksum. If the checksum does not match, then the above rule to go into download mode applies. If the checksum does match, then it looks at the last time it saw a valid Pull. If that was less than 1s ago, then it relies back with an Im Still Seeding packet. If the past Pull packet was received more than 1s ago, then the Seeding timer would have already expired so the Seed packet is ignored since the active game has already been started.

Anytime a blink gets back a Pull or Im Still Seeding packet, it resets it own Seed timeout.  This has the effect of keeping all connected blinks in Seed mode until all downloads are finished. When the blink that started the Seed (by 3 second button press) finally times out, then it knows that all connected blinks have the new game downloaded and started. It can now optionally broadcast a viral "let all start now!" packet on all faces for a brief amount of time if it wants to have all blinks start at about the same time after a download. Any blink getting this viral packet also starts broadcasting it for the time period so it spreads to all connected blinks, signaling them to also start the game at the end of the period. If you want to get fancy, you might even be able to count the hops in this packet and use that info to have all blinks start almost simultaneously. 
   

Note that even Seeding mode, the above rule still applies and if the blink sees another Seeding packet come from elsewhere. 
  
When a blink sees a Seeding packet on any face, it goes into downloading mode and starts sending Pull packets tot he face where it saw the Seeding packet. 

When the bootloader starts, it check checks if the button is down. If it is, then it waits for the button to be lifted. If the button is lifted within 3 seconds, then the blink copies the built-in game to the active area and starts seeding as described below. If the button is helped down for 3 seconds, then the blink goes to sleep. 

 

starts looking for Pull Request packets on all faces. If is sees one within the first 1 second, then it will begin downloading the new game from that face by sending Pull packets. It will continue downloading the new game into the active area until it is completely downloaded or until there is a timeout. If the game is successfully downloaded, then the Blink will start the newly downloaded game in the active area. If the blink times out while downloading, then it will copy the built-in game to the active game area and start it.

A blink initiates sending a game by transmitting Pull Request packets on all faces. It will answer any Pull packets it gets with Push packets containing pages from the game currently in the active area. If it goes for more than 1 second without seeing a valid Pull packet then it will restart the currently active game.   

When a Blink gets a Pull Request packet for a game that does not match the game in its active area, then it will reboot into the bootloader which will then start looking for Pull Request packets.

When a Blink gets a Pull Request packet for a game that does match the game in its active area, it will reply with either an Im Seeding packet if it has sent a push withing the last timeout period, or a Ready to Rock packet if it has not sent a Push packet in the timeout period (because all adjacent blinks have already switched to Ready To Rock). 

The Im Seeding packet is really just a place holder so that the adjacent 

Note that this means that after a blink has already downloaded a game, it will see Pull Request packets that match its current checksum, so it will not download the game again. The sending blink will then timeout when all of the blinks on all of its faces have downloaded the new game. It is up to the game code to decide what to do on start up, it would either launch straight into the game even though there still might be distant downloads in progress, or it could show a "waiting to start animation" and 


## Game checksums

To compute the game checksum, you take the individual checksum for each page and add the page number to that, and then sum all of these totals for all of the pages. This helps detect out-of-order problems that a straight checksum without the page number added might miss. 

## Optimizing game delivery across a field of Blinks

Note that  Blinks on consecutive sides also share sides with each other, so we try not to send the Pull Request packets on consecutive sides to try and distribute the downloads. This way one the first Blink gets the new game, it can try to send to the neighbor.   

## Send game sequence (bootloader cold load)

1. User does a 3 second button press and the blinkOS then does a hard reset into the bootloader. Notice this is also what the blink does on hardware reset or battery change. 
2. The bootloader copies the built-in game to the active area and starts it.
3. The active game starts seeding as shown below in BlinkOS startup. 

Note that a Blink can only ever send its own built-in game via user action (button press, hard reset, battery change). This is sort of DRM that does not let you viraly copy a game since you need the tile that has that game built-in to start it spreading. It is also necessary since it assures that a Blink always starts in a known good fresh state after reset or send request.

Note that this copy routine must be in the bootloader since it writes to flash, but it is not part of the normal IR download code.  

Note that the BOOTRST fuse must be set so that power up and reset events start the bootloader and not the active game.    

## Blink OS start up

BlinkOS always starts up in seed mode where it will try to transfer the active game to nearby Blinks. 

1. Broadcast Pull Request packets with active game checksum on all sides. 
2. Answer any Pull packets that match our checksum with push.

If we do not see any Pull packets for some timeout, then we start the game. This should stay in sending mode for as long as there are neighbors that do not have the same game a us.  

The game can wait for some game defined action to decide when to actually start playing. Could be a packet or a button press.
 
## Bootloader warm load

When the active game sees a Pull Request packet, it jumps into the bootloader's "load" mode, passing the checksum and face of the Pull Request packet.

The bootloader starts sending Pull packets on that face and processing the resulting Push packets. 

If there is a timeout waiting for a valid Push packet or if the download completes with a bad total checksum, then the bootloader does a hard reset - which reloads and starts seeding the built-in game. This is problematic because now this built-in game seeding will start competing with the game that failed to download, but at least this is deterministic in that we always end up with a good active game loaded everywhere. Otherwise if we kept waiting for a good download there would be no good way for the user to abort this. It would be nice for them to do a long button press, but so far not enough room for buttons in the bootloader.

TODO: On failed download attempt, start hunting for new Pull Request packets on all faces until you see one and get a good download. ONly way out of this is a long long button press to load built-in game.   

1. Compute checksum for active game.
2. Compare that checksum to the 
2. Look around for 1 second for a Pull Request 

## When to activate the built in game

1. On cold power up after battery change or manual pin reset
2. When user holds button down for 3 seconds. The active game will jump into the bootloader with a "reset" flag set and the bootloader will copy the built-in game down to the active area.
3. We started a download but had a timeout. At this point we do not know if the download is good, so have to have something useful in the active area. 


Whenever we have copied the built-in game down, we always start it in seed mode.


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

At startup, the chip starts executing the bootloader reset vector `0x1c00`. The requires the `BOOTRST` fuse to be programmed to `0` (not the default). If the bootloader decides that it does not need to load anything (or after it is done loading) then it jumps to `0x000` to start the application code. 


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