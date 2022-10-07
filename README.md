# bad-bootloader
My first attempt at making some kind of simple bootloader.
Loads `kernel.bin` from first FAT32 partition on USB/HDD (must be LBA compatible)
## TODO:
1. Right now the MBR is hardcoded to MBR of my USB, I will have to make some kind of Python script to install bootloader on device and keep original MBR
2. Rewrite FAT32 driver part