# AGENTS.md

This file provides guidance to AI coding agents when working with code in this repository.

**All code, comments, commit messages, PR descriptions, and documentation must be written in English.**

## Project Overview

WuaDisplay_LMX is a PlatformIO Arduino library (ESP32) that provides a unified, high-level display API on top of different hardware backends:

- **LMX1** — N WuaDisplay modules (7×18 px each) chained horizontally, driven as SK6812 addressable LEDs via FastLED.
- **LMX2** — AW20216S RGB LED matrix (6×12 px, SPI).

It is meant to be the consolidated base library other projects depend on (e.g. `WuaDisplay-r1-firmware`, which chains 7 LMX1 modules into one large display).

Dependencies (per backend): `adafruit/Adafruit GFX Library` (both), `fastled/FastLED` (LMX1), `Wualeds_AW20216S` (LMX2).

## Architecture

The library is organised in **two layers** using **static (compile-time) polymorphism**:

- **Layer 1 — pixel panels** (`LMX1`, `LMX2`): each is an `Adafruit_GFX` subclass implementing the pixel primitive `drawPixel()` plus a small contract: `begin()`, `clear()`, `flush()`, `blur()`. They own the physical layout — e.g. `LMX1` maps a logical `(7·N)×18` canvas onto the right-to-left LED chain.
- **Layer 2 — high-level engine** (`WuaDisplayLMX<Panel>`, header-only template): text alignment, non-blocking scroll, effects and color helpers, written once on top of any panel's GFX primitives. It composes the panel **by value** (no heap, no vtable on the hot path).

Two axes of variation, two mechanisms:
- **Backend type → compile time.** `WuaDisplay_LMX.h` selects the panel from a build flag (`WUA_BOARD_LMX1` / `WUA_BOARD_LMX2`) and exposes the concrete alias `WuaDisplay`.
- **Module count → runtime.** Forwarded to the panel constructor (`WuaDisplay display(7);`).

Source layout:
- `src/WuaDisplayLMX.h` — Layer 2 engine (template) and the Panel contract documentation.
- `src/LMX1.{h,cpp}`, `src/LMX2.{h,cpp}` — Layer 1 backends.
- `src/WuaDisplay_LMX.h` — public umbrella header; compile-time backend selection → `using WuaDisplay = ...`.
- `src/main.cpp` — example sketch / build harness.

## Build

This repo doubles as a PlatformIO library and a build harness. One environment per backend:
- `pio run -e lmx1` — LMX1 (FastLED).
- `pio run -e lmx2` — LMX2 (AW20216S).
- `pio run -e <env> -t upload` to flash.

`build_src_filter` excludes the inactive panel so only the active backend's dependency is compiled. When consumed as a library, define the board flag in the consuming project's `platformio.ini`.

## Code Style

- CamelCase class names, camelCase methods, underscore-prefixed private members
- All comments in English (see the rule at the top of this file)
- Layer 1 panels are `Adafruit_GFX` subclasses; keep the Panel contract (`begin/clear/flush/blur` + `drawPixel`) intact so the Layer 2 template stays backend-agnostic
