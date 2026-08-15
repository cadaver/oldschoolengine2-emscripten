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
- Raster interrupt + partial CIA1 Timer A emulation
- Joystick port 2 control with arrows + ctrl as fire button
- Keyboard input
- D64 & D81 image support, loading / saving via minimal (and incorrect) Kernal routine traps
- Save file persistence

TODO:

- Full key mappings

Licensed under the MIT license, see the code for details. Use at own risk.

## Building

To build, the Emscripten SDK and CMake are needed. Ensure the EMSDK environment is set up correctly in your shell and type (for a release build, change as needed):

    emcmake cmake . -DCMAKE_BUILD_TYPE=Release
    make

## Startup options

The emulator allows a diskimage query parameter. By default the Steel Ranger demo (included) is run, but to run Hessian instead, assuming a localhost page over http:

    http://127.0.0.1:yourport/oldschoolengine2.html?diskimage=hessian
