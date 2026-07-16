# Marlin 2.1.x — Ender-3 V3 SE Custom Firmware

This is a community fork of [Marlin 2.1.x bugfix](https://github.com/MarlinFirmware/Marlin) tailored for the **Creality Ender-3 V3 SE** (CR4NS200320C13 / C14 boards). See [README.md](../README.md) for the full feature list and branch map.

## Target Hardware

| Item | Value |
|------|-------|
| Board (C13) | `BOARD_CREALITY_V3_GD303` — STM32F103RE (512 KB Flash, 64 KB RAM) |
| Board (C14) | STM32F401 variant |
| Serial | UART1, 115200 baud |
| EEPROM | IIC BL24CXX, 2 KB @ 0x800 |
| Flash offset | `0x7000` (bootloader) |

## Branch Map

| Board | Use case | Branch |
|-------|----------|--------|
| C13 | SD card only | `for_E3V3SE` |
| C14 | SD card only | `for_STM32F401` |
| C13 | OctoPrint / serial host | `for_Octoprint` |
| C14 | OctoPrint / serial host | `for_Octoprint_F401` |

## Build

Uses [PlatformIO](https://platformio.org/). The default environment is `STM32F103RET6_creality`.

```bash
# Build default (C13 board)
pio run -e STM32F103RET6_creality

# Build C14 variant
pio run -e STM32F401RC_creality

# Build all environments
make marlin           # runs ./buildroot/bin/mftest -a
```

Environment definitions live in [platformio.ini](../platformio.ini) and [ini/stm32f1.ini](../ini/stm32f1.ini).

## CRITICAL: Memory Constraints

**Every feature addition must stay within these limits:**
- RAM: **< 40%** of 64 KB (≈25 KB)
- Flash: **< 46%** of 512 KB (≈235 KB) for C13 board

Check usage after building via **PlatformIO Home → Project Inspect → Advanced Memory Usage**.  
Features are annotated with approximate flash costs in [Configuration.h](../Marlin/Configuration.h) comments.

## Configuration

All firmware settings are in two files:

- [Marlin/Configuration.h](../Marlin/Configuration.h) — board, geometry, sensors, LCD, basic features
- [Marlin/Configuration_adv.h](../Marlin/Configuration_adv.h) — advanced motion, thermal, host support, EEPROM

Features are enabled/disabled with `#define` / `// #define`. Do **not** edit generated files; only edit these two.

## Key Source Directories

| Path | Purpose |
|------|---------|
| `Marlin/src/lcd/dwin/creality/` | Custom DWIN display driver for E3V3SE (`dwin.cpp`, `dwin_lcd.cpp`, `ui_position.h`) |
| `Marlin/src/lcd/e3v2/common/` | Shared DWIN protocol API |
| `Marlin/src/feature/` | Optional features: runout, babystep, leveling, TMC, input shaping |
| `Marlin/src/gcode/` | G-code command implementations |
| `Marlin/src/module/` | Core systems: planner, stepper, temperature, motion, settings |
| `Marlin/src/HAL/STM32F1/` | HAL for this board's MCU family |
| `Marlin/src/pins/stm32f1/` | Pin definitions (`pins_CREALITY_V3_GD303.h`) |

## Tests

```bash
# Unit tests (native simulator)
make unit-test-all-local
pio test -e native_sim

# Integration tests
make tests-all-local

# Pin validation
make validate-pins
```

Test configs live in [test/](../test/) (`001-default.ini`, `002-extruders_1_runout.ini`, etc.).

## Custom Features (E3V3SE-Specific)

- **DWIN Thumbnail**: `DWIN_RENDER_THUMBNAIL` — renders G-code embedded thumbnails on display
- **One Click Print**: `ONE_CLICK_PRINT` — auto-start last print from SD
- **Delta/X Z-Offset Routine**: custom auto Z-offset calibration
- **Input Shaping / FT_Motion**: reduce ringing (replaces standard Input Shaper in some branches)
- **Linear Advance**: pressure advance compensation
- **HX711 Load Cell**: filament weight sensor (PA4 clk, PC6 data)
- **CRTouch**: BLTouch-compatible probe for mesh leveling
- **LCD Dimmer & Brightness**: custom M-codes for display control
- **Compact 6×6 Grid**: Eduard's bed mesh leveling grid

## Firmware Binaries

Pre-built `.bin` files for C13 board are in [FW_HWC13/](../FW_HWC13/). Flash via SD card by renaming to `firmware.bin`.

## Serial Debugging

[Serial_Debugger/serial_util.py](../Serial_Debugger/serial_util.py) — utility for serial communication debugging with the printer.

## Conventions

- C++ code follows the [Marlin style guide](contributing.md)
- Prefer `#if ENABLED(FEATURE)` / `#if DISABLED(FEATURE)` macros over raw `#ifdef`
- Use `PROGMEM` and `PSTR()` for string literals on AVR (not needed for STM32 but keep portable)
- Feature guards: always wrap new features in `#if` blocks with a clear `#define` in `Configuration.h`
- Memory is the primary constraint — measure before committing any new feature
