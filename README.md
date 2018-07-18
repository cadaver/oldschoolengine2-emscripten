# OldschoolEngine 2 Emscripten port

Minimal line-based Commodore 64 emulator that emulates just enough to run the recent Covert Bitops C64 games (Hessian, Steel Ranger...) 
This is a C++ / Emscripten port that runs on web pages.

The original "oldschoolengine" ran on GameBoy Advance to run Metal Warrior 4, and it used a custom API for graphics, sound and file access. In contrast,
this project emulates a limited subset of an actual C64, so that the game can run unmodified.

Features:

- CPU emulation based on EMU6502 code by Yve Verstrepen
- Parts of SID emulation (noise, filter) based on jsSID by Mihaly Horvath
- Emscripten build system & OpenGL initialization based on tiny_chess by Jukka Jylänki
- Line-based VIC-II rendering
- Raster interrupts. No actual CIA chip emulation
- Joystick port 2 control with arrows + ctrl as fire button
- Keyboard input
- D64 & D81 image support, loading / saving via minimal (and incorrect) Kernal routine traps

TODO:

- Full key mappings
- Persistent save of files written to the disk image (save games)

Licensed under the MIT license, see the code for details. Use at own risk.

## Building

To build, the Emscripten compiler (emcc) and CMake are needed. Invoke CMake by specifying the generator and path to Emscripten's CMake toolchain.

`cmake . -G"Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=MyPathToEmscripten.cmake`