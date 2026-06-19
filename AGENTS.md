# AGENTS.md

This file provides guidance to AI coding agents when working with code in this repository.

**All code, comments, commit messages, PR descriptions, and documentation must be written in English.**

## Project Overview

WuaDisplay_LMX is a PlatformIO Arduino library (ESP32) that provides a unified, high-level display API on top of different hardware backends:

- **LMX1** — N WuaDisplay modules (7×9 px each) chained horizontally, driven as SK6812 addressable LEDs via FastLED.
- **LMX2** — AW20216S RGB LED matrix (6×12 px, SPI).
- **LMX2d** — one PCB carrying **two** AW20216S chips side by side, sharing a single SPI bus with one chip-select line each, exposed as a single 12×12 px canvas. It is *not* two chained LMX2 boards: the two 6×12 halves are integrated on the same PCB.

It is meant to be the consolidated base library other projects depend on (e.g. `WuaDisplay-r1-firmware`, which chains 7 LMX1 modules into one large display).

Dependencies (per backend): `adafruit/Adafruit GFX Library` (all), `fastled/FastLED` (LMX1), `Wualeds_AW20216S` (LMX2 and LMX2d).

## Architecture

The library is organised in **two layers** using **static (compile-time) polymorphism**:

- **Layer 1 — pixel panels** (`LMX1`, `LMX2`): each is an `Adafruit_GFX` subclass implementing the pixel primitive `drawPixel()` plus a small contract: `begin()`, `clear()`, `flush()`, `blur()`. They own the physical layout — e.g. `LMX1` maps a logical `(7·N)×9` canvas onto the right-to-left LED chain.
- **Layer 2 — high-level engine** (`WuaDisplayLMX<Panel>`, header-only template): text alignment, non-blocking scroll, effects and color helpers, written once on top of any panel's GFX primitives. It composes the panel **by value** (no heap, no vtable on the hot path).

Two axes of variation, two mechanisms:
- **Backend type → compile time.** `WuaDisplay_LMX.h` selects the panel from a build flag (`WUA_BOARD_LMX1` / `WUA_BOARD_LMX2` / `WUA_BOARD_LMX2D`) and exposes the concrete alias `WuaDisplay`.
- **Hardware setting → runtime.** Forwarded to the panel constructor: module count for LMX1 (`WuaDisplay display(7);`), CS pin for LMX2 (`WuaDisplay display(10);`), the two CS pins for LMX2d (`WuaDisplay display(10, 1);`).

Source layout:
- `src/WuaDisplayLMX.h` — Layer 2 engine (template) and the Panel contract documentation.
- `src/LMX1.{h,cpp}`, `src/LMX2.{h,cpp}`, `src/LMX2D.{h,cpp}` — Layer 1 backends. `LMX2D` reuses `LMX2`'s geometry macros (`LMX2_WIDTH`/`LMX2_HEIGHT`) and routes each pixel to the chip that owns its column (`x < LMX2_WIDTH` → left, else right).
- `src/WuaDisplay_LMX.h` — public umbrella header; compile-time backend selection → `using WuaDisplay = ...`.
- `examples/` — reference sketches written against the LMX1/LMX2 backends (the library ships **no** app entry point). LMX2d shares the same high-level API; CI smoke-compiles it from a minimal sketch instead.

Each backend's translation unit is wrapped in its board flag: `LMX1.cpp` compiles only in the LMX1/default case (neither `WUA_BOARD_LMX2` nor `WUA_BOARD_LMX2D` set), `LMX2.cpp` only under `WUA_BOARD_LMX2`, `LMX2D.cpp` only under `WUA_BOARD_LMX2D`. So a consumer compiles — and pulls the dependency of — only the active backend, regardless of `build_src_filter`. Keep these guards consistent with the `#if/#elif/#else` in `WuaDisplay_LMX.h`.

## Build

This is a library, not an app: `src/` ships no entry point, so `pio run` cannot link it on its own. Consume it by adding it as a dependency and selecting a backend with a build flag (`-D WUA_BOARD_LMX1`, `-D WUA_BOARD_LMX2`, or `-D WUA_BOARD_LMX2D`).

To compile-check a bundled example (what CI does), build it as the sketch through the repo's environments, e.g.:

```sh
cp examples/Basic/Basic.ino src/main.cpp && pio run -e lmx2 && rm src/main.cpp
```

CI (`.github/workflows/build.yml`) builds every `examples/` sketch (LMX2 env) plus a minimal LMX1 sketch and a minimal LMX2d sketch on each push/PR.

## Code Style

- CamelCase class names, camelCase methods, underscore-prefixed private members
- All comments in English (see the rule at the top of this file)
- Layer 1 panels are `Adafruit_GFX` subclasses; keep the Panel contract (`begin/clear/flush/blur` + `drawPixel`) intact so the Layer 2 template stays backend-agnostic
