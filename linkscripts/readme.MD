We use a shared data block for communication between the bootloader code and the foreground game. This lets us load that stuff once rather than having to include it in every game image. 

The shared memory block lives at a fixed location at the bottom of RAM. 

We need a custom link script so we can force that block to this known location.

The linker scripts here were copied from...

C:\Program Files (x86)\Arduino\hardware\tools\avr\avr\lib\ldscripts

...which seems to be hardcoded into `gcc`. 

We add a new group of sections called `ipcram1`-`ipcram9` which hold the shared memory block. This comes at the beginning of the the `data` segment so should land right at 0x100 every time (I hope?).

Note that we move the `__data_start` symbol *after* our blocks since this symbol is used by the startup code to initialize static variables from flash and we do not Want our block to get initialized (or overwritten). We will manually initialize it in the bootloader.  

Note also that we mark these sections as `KEEP`, but they still seem to get garbage collected if not referenced, so we should make sure to reference them in the foreground code or else the foreground will put variables here and overwrite the bootloader's info. 


To get Arduino use this custom script, we edit the `platform.txt` to have...

```
recipe.c.combine.pattern="{compiler.path}{compiler.c.elf.cmd}" -Wl,-verbose -Wl,--script="{{build.core}/linkerscripts/avr5.xn" {compiler.c.elf.flags} -mmcu={build.mcu} {compiler.c.elf.extra_flags} -o "{build.path}/{build.project_name}.elf" {object_files} "{build.path}/{archive_file}" "-L{build.path}" -lm
```

The `-Wl,` means "pass the next arg to the linker and the `--script=` [specifies the linker script](ftp://ftp.gnu.org/pub/old-gnu/Manuals/ld-2.9.1/html_chapter/ld_2.html#SEC3). This sucks that we have to hard code the `avr5` in there but I think the mapping from MCU to those `avrX` code is hardcoded into gcc.


To put a variable into this new section, we use...

```
byte __attribute__((section ("ipcram1"))) counter=0;
```

More info:
https://gcc.gnu.org/onlinedocs/gcc-3.2/gcc/Variable-Attributes.html

