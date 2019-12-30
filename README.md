MultiSimZ80
===========

Multi-Platform Z-80/TRS-80 Simulator, currently supporting Arduino Due and ESP32-WROVER-KIT

Features
--------
- Zilog Z-80 Simulator with support for David Keil's extended instructions (partly implemented) and my own extensions (emulated cassette I/O, terminal I/O, debugging)
- read-only file system for cassette files using PROGMEM
- SD file system for cassette and disk images
- TRS-80 Model I with SS/SD disk system emulation and 32kB RAM
- 16x64 Character LCD screen support
- integrated mini-debugger and disassembler
- separate thread for terminal I/O (screen update) on ESP32


To Do Next
----------
- Keyboard interfacing, PS/2, USB or 8x8 key matrix


To Do Later
-----------
- Sound output
