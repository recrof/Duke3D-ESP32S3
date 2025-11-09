# MGM240 DukeNukem3D a port to Silicon Labs EFR32MG24/MGM240 devices requiring only 256 kB RAM!

This is a port to the Gamepad, based on the Arduino Nano Matter board.
It does not require external RAM: it uses just the built-in 256 kB RAM. Two external flash ICs are used to store the DUKE3D.GRP data file.

Based on Chocolate Duke 3D. Port by Nicola Wrachien.

For more information visit the technical article on: https://next-hack.com/index.php/2025/11/14/duke-nukem-3d-on-the-arduino-nano-matter-board-only-256-kb-ram/.

## Features

* Full engine and game logic implementation:

  * Sloped ceilings, floors.
  * Security Cameras.
  * Mirrors.
  * Destroyable environment.
  * Movable sectors.
  * translucent sprites.
  * Screen tilting.
  * Map.
  * Sector-based lighting.
  * Z-depth lighting.

* Resolution: 320 x 240 pixels (20% larger than default resolution)
* Sound and music:

  * 8 bit stereo, 11025 Hz.
  * Reverb.
  * Music with OPL2 emulation.

* Console with cheats.
* Options (saved on external flash), with keyboard and thumbstick remapping.
* Savegames; full game state support.
* Support for full game GRP file (requires 2x16 MB flash ICs) and Atomic edition (requires 2x32 flash ICs).
* Wireless multiplayer over 802.15.4, with configuration.
* Average 37 fps on E1L1.



## Known Bugs

* Changing options during multiplayer will corrupt data. You will have to clear the options on next boot!

## Supported Boards

* The Gamepad (described in [this article on next-hack.com](https://next-hack.com/index.php/2024/09/21/the-gamepad-an-open-source-diy-handheld-gaming-console)), based on [Arduino Nano Matter Community Preview Board](https://store.arduino.cc/en-hu/pages/nano-matter).

**You can easily add support to more boards by editing boards.h and creating new build configurations.**

## Build and programming

See article cited above.

## DUKE3D.GRP file upload

We suggest to buy from Zoom-Platform the Duke3D atomic edition for few bucks, and get the Duke3D.GRP file (https://www.zoom-platform.com/product/duke-nukem-3d-atomic-edition). Shareware Duke3D.GRP is not tested. Original Duke3D.GRP is supported and works, but it is hard to find a legal copy nowadays.

See article on next-hack.com cited above.

## Changelog

V1.0: Initial release.

