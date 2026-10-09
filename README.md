# Duke3D-ESP32S3: Duke Nukem 3D for ESP32-S3 boards

Port of the MGM240 Duke Nukem 3D by Nicola Wrachien (Chocolate Duke 3D based)
to ESP32-S3 boards with 16 MB flash and 8 MB octal PSRAM:

| Board | Display | Input | Sound |
|-------|---------|-------|-------|
| [LilyGO T-Deck / T-Deck Plus](https://github.com/Xinyuan-LilyGO/T-Deck) | 2.8" ST7789 (SPI) | keyboard, trackball | MAX98357A speaker |
| Seeed Studio Wio Tracker L2 / L2 Pro | 3.2" NV3031B (QSPI) | touch screen (GT911), BOOT/WAKE buttons | ES8311 codec, speaker |

On both, a Bluetooth LE gamepad (e.g. Xbox Wireless Controller) can be used too.

The engine and game sources (`Duke3D-ESP32S3/Engine`, `Duke3D-ESP32S3/Game`) are
shared with the MG24 port, whose Simplicity Studio project and platform files
are still in `Duke3D-ESP32S3` (`Port/`, `autogen/`, `config/`, `MGM240DN3D.slcp`).
The ESP32-S3 platform layer is in `Duke3D-ESP32S3/main`.

## Build and flash

Requires [PlatformIO](https://platformio.org/). The project pins the official
`espressif32@6.11.0` platform (ESP-IDF 5.4.1).

```sh
cd Duke3D-ESP32S3
pio run -e tdeck -t upload       # LilyGO T-Deck
pio run -e wio_l2 -t upload      # Seeed Wio Tracker L2 / L2 Pro
pio device monitor               # optional: boot log and frame times
```

With plain ESP-IDF: `idf.py -B build-l2 -DDUKE_BOARD=BOARD_WIO_TRACKER_L2 build`
(`BOARD_TDECK` is the default). Pins and per-board settings are in
`main/port/boards.h`, bring-up and input devices in `main/board_<name>.c`.

If the upload fails, put the board in download mode: hold BOOT (on the T-Deck:
press the trackball) while switching it on or resetting it. Afterwards
power-cycle **without** holding it.

## Game data

Copy `DUKE3D.GRP` to the root of a FAT32 microSD card. Tested with the Atomic
Edition (44 MB); the file is read straight from the card through a PSRAM cache,
nothing is installed to flash.

The MCU engine needs its sounds as 8-bit WAV and its music as single-track MIDI
(what `Tools/PythonCVT/mcugrpcvt.py` produces for the MG24 board). On first
boot the board does this itself: it writes `DN3DCONV.GRP` next to the original
(about 3 minutes, needs ~45 MB free) and uses it from then on. The original file
is never modified; replacing `DUKE3D.GRP` triggers a new conversion. A GRP that
was already converted on a PC is used as is.

Settings and savegames are kept in the `dukenvm` flash partition. To erase
them, hold **Alt + Space** (T-Deck) or **WAKE + a finger on the screen**
(Wio Tracker L2) while the boot text is shown.

## Controls

### T-Deck

| Action              | Key                                   |
|---------------------|---------------------------------------|
| Move / turn         | W S / A D                             |
| Strafe              | Q / E                                 |
| Turn and look       | trackball                             |
| Fire                | trackball click, or Alt               |
| Open / use          | Space                                 |
| Jump / crouch       | X / Z                                 |
| Quick kick          | C                                     |
| Run                 | Shift                                 |
| Aim up / down       | T / G                                 |
| Weapons             | Sym + key (the digit printed on it)   |
| Next / prev. weapon | P / O                                 |
| Inventory use       | Enter (K / L to select)               |
| Map                 | Mic                                   |
| Menu (Escape)       | `$`                                   |
| Console (cheats)    | Sym + Q (`#`)                         |

In menus the trackball moves the cursor and its click selects. All keys can be
rebound in the game's setup menu.

Held keys need the keyboard's raw mode, available in current LilyGO keyboard
firmware. With older firmware every key press is a short tap.

### Wio Tracker L2 (touch screen)

| Action              | Touch                                              |
|---------------------|----------------------------------------------------|
| Move / strafe       | left half: virtual stick where the finger lands    |
| Turn and look       | drag on the right half (or from FIRE)              |
| Fire                | **F** button                                       |
| Open / use          | **U** button                                       |
| Jump / crouch       | **J** / **C** buttons                              |
| Next weapon         | **W** button                                       |
| Menu (Escape)       | top left corner, or the BOOT button                |
| Inventory use       | WAKE button                                        |

In menus swipe up/down/left/right to move, tap to select (or answer "yes"),
and tap the top left corner (or press BOOT) to go back. The on-screen buttons
are hidden while a Bluetooth gamepad is connected, until the screen is touched.

### Bluetooth gamepad

Put the pad in pairing mode near the board: it is found, bonded and from then
on reconnects by itself.

| Pad                    | Game                          |
|------------------------|-------------------------------|
| Left stick             | move / strafe                 |
| D-pad                  | move / turn (menus: navigate) |
| Right stick            | turn / look                   |
| RT / LT                | fire / run                    |
| A / B                  | jump / crouch (menus: select / back) |
| X / Y                  | open / use inventory item     |
| LB / RB                | previous / next weapon        |
| View / Menu            | map / menu                    |
| Left / right stick click | quick kick / center view    |

## How it maps to the MG24 port

| MG24                                   | ESP32-S3                                        |
|----------------------------------------|-------------------------------------------------|
| GRP in two interleaved SPI flash chips | GRP on SD card, 16 kB pages cached in PSRAM     |
| MCU internal flash level cache         | 2 MB PSRAM arena (`internalFlash.c`)            |
| settings/savegames at the end of flash | `dukenvm` flash partition, same NOR semantics   |
| display DMA raced by the renderer      | frame snapshot pushed by a core-0 task          |
| PWM/DAC audio from an LDMA ring        | same ring drained to I2S                        |
| gamepad shift registers + joysticks    | keyboard/trackball, touch screen, BLE gamepad   |
| 802.15.4 multiplayer                   | not available                                   |

# The original MGM240 port

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

