We use a shared data block for communication between the bootloader code and the foreground game. This lets us load that stuff once rather than having to include it in every game image. 

The shared memory block lives at a fixed location at the bottom of RAM. 

We need a custom link script so we can force that block to this known location.

The linker scripts here were copied from...

C:\Program Files (x86)\Arduino\hardware\tools\avr\avr\lib\ldscripts

...which seems to be hardcoded into `gcc`. 

We add a new group of sections called `ipcram1`-`ipcram9` which hold the shared memory block. 

Note also that we mark these sections as `KEEP`, but they still seem to get garbage collected if not referenced, so we should make sure to reference them in the foreground code or else the foreground will put variables here and overwrite the bootloader's info. 

Note that under the current system, the BIOS code can not have any static variables since these would live in `.bss` or `.data` and those sections will get overwritten when the user code runs. I've defined canary sections called `.nobss` and `.nodata` each with length 0 so if some data accidentally gets into the BIOS code, at lest you will get the error `BlinkBIOS.elf section `.nodata' will not fit in region `nodata'region `nodata' overflowed by X bytes`.

If you do need static storage for variables in the BIOS code, stick it into once of the `ipcram` blocks. There is even a `slack` block with room for the future in case you need it. 

Note that I tried messing with the `.data` section, but that messed up the `__stack` value and I could not figure out where that was coming from, so don't mess with it. 

The new script is much cleaner than the stock one, and also moves the `bss` segment down so that both the `ipcram` memory and the normal static variables can all be initialized to `0` in one big block. Note that we only want to zero out the `ipcram` in the BIOS startup, so the linker script for user programs will put the `bss` start above here.  

To get Arduino use this custom script, we edit the `platform.txt` to have...

```
recipe.c.combine.pattern="{compiler.path}{compiler.c.elf.cmd}" -Wl,-verbose -Wl,--script="{{build.core}/linkerscripts/avr5.xn" {compiler.c.elf.flags} -mmcu={build.mcu} {compiler.c.elf.extra_flags} -o "{build.path}/{build.project_name}.elf" {object_files} "{build.path}/{archive_file}" "-L{build.path}" -lm
```

But it is better to pre-compile this BIOS code into a HEX file and then have the Arduino code blindly flash it using AVRDUDE during the download. To compile in AS7, we have to add this to the `Toolchain->Linker->Misc->Linker flags`...

`-Wl,--script="../../../src/linkscripts/avr5.xn"`

The `-Wl,` means "pass the next arg to the linker and the `--script=` [specifies the linker script](ftp://ftp.gnu.org/pub/old-gnu/Manuals/ld-2.9.1/html_chapter/ld_2.html#SEC3). This sucks that we have to hard code the `avr5` in there but I think the mapping from MCU to those `avrX` code is hardcoded into gcc.


To put a variable into this new section, we use...

```
byte __attribute__((section ("ipcram1"))) counter=0;
```

More info:
https://gcc.gnu.org/onlinedocs/gcc-3.2/gcc/Variable-Attributes.html

