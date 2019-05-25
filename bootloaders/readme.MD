This folder contains the BlinkBIOS bootloader HEX file. This file is created by the BlinkBIOS repo...

https://github.com/bigjosh/Move38-BlinkBIOS

..and then copied here after each compile by an Atmel Studio post-build event. Note that the this bootloader directory is hardcoded into the text of that event. 

This file is used by...

1. The `upload` and `program` recipes to download a new sketch to a blink. Note that we could make upload only update the actual program and not the BIOS for faster turnaround during development, but I do not want to change this behavior now. The path to the bootloader is hardcoded into the recipes but is relative to the platform folder.
2. The `Export Binary` function which creates a single HEX file with both the current sketch and the BIOS. In this case, the folder name must be `bootloader` as this is hardcoded into the IDE. 


 