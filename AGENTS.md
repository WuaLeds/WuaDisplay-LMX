# AGENTS.md

This file provides guidance to AI coding agents when working with code in this repository.

## Project Overview

WuaDisplay_LMX is a PlatformIO Arduino library (ESP32) that provides a unified display driver for three hardware module types:

- **MOD_WuaDisplay** — Intelligent WuaDisplay modules (7 modules, uses `WuaDisplay` class)
- **MOD_LMX1** — Single LMX module (1 module, uses `WuaDisplay` class)
- **MOD_LMX2** — AW20216S RGB LED matrix (6×12 pixels, SPI, uses `AW20216S` class)

Dependencies: `Wualeds_AW20216S`, `WuaDisplay`, Arduino core.

## Architecture

The library uses a **facade/adapter pattern** — a single `WuaDisplay_LMX` class wraps three different display backends behind a unified interface (`begin()`, `clear()`, `printAligned()`). Method dispatch is switch-based on the `ModuleType` enum set at construction time. Each backend is heap-allocated and stored as a private pointer.

All source lives in `src/WuaDisplay_LMX.h` and `src/WuaDisplay_LMX.cpp`.

## Build

This is a PlatformIO library, not a standalone project. To use it:
- Add it as a dependency in a consuming project's `platformio.ini`
- Build/flash from the consuming project with `pio run` / `pio run -t upload`

## Code Style

- CamelCase class names, camelCase methods, underscore-prefixed private members
- Comments are mixed Spanish and English
