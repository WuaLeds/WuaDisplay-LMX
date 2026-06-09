# WuaDisplay-LMX

A unified, high-level display library (PlatformIO / Arduino, ESP32) for WuaLeds
display hardware. It exposes one text / scroll / effects API on top of different
pixel backends, and is meant to be the consolidated base library other projects
depend on.

## Architecture

Two layers, joined by **static (compile-time) polymorphism** — no virtual
dispatch and no heap on the hot path.

```
┌─ Layer 2 · HIGH LEVEL (generic, written once) ────────────────────┐
│  WuaDisplayLMX<Panel>                                              │
│  aligned text · non-blocking scroll · effects · color helpers      │
│  (built on the panel's Adafruit_GFX primitives)                    │
└────────────────────────────────────────────────────────────────────┘
                          ▲ composes a Panel by value
┌─ Layer 1 · PIXEL PANEL (Adafruit_GFX subclass) ───────────────────┐
│  contract: drawPixel() · begin() · clear() · flush() · blur()      │
│  ► LMX1 : FastLED, N×(7×18) SK6812 modules chained right-to-left   │
│  ► LMX2 : AW20216S 6×12 RGB matrix (SPI), 565→RGB adapter          │
└────────────────────────────────────────────────────────────────────┘
```

Two axes of variation, two mechanisms:

| Axis            | When        | How                                              |
|-----------------|-------------|--------------------------------------------------|
| Backend type    | compile time | build flag `WUA_BOARD_LMX1` / `WUA_BOARD_LMX2`  |
| Module count    | runtime      | constructor argument (`WuaDisplay display(7);`)  |

The pixel base is `Adafruit_GFX`: a panel only implements `drawPixel()` plus the
small `begin/clear/flush/blur` contract, and the whole text/scroll/effects layer
is written once on top.

## Backends

- **LMX1** — N modules of 7×18 px chained horizontally, driven as SK6812
  addressable LEDs through FastLED. Logical canvas: `(7·N) × 18`.
- **LMX2** — single AW20216S 6×12 RGB matrix over SPI. (At 6 px wide, text is
  barely legible; this panel mainly targets pixel art and effects, but honours
  the same contract.)

## Usage

```cpp
#include <Arduino.h>
#include "WuaDisplay_LMX.h"

// `WuaDisplay` resolves to the backend selected by the build flag.
WuaDisplay display(7);   // LMX1: 7 chained modules  (LMX2: CS pin, e.g. display(10))

void setup() {
    display.begin();
    display.setTextColorRGB(80, 50, 80);

    display.clear();
    display.printAligned("Hi");
    display.flush();

    display.startScroll("Hello WuaLeds!", 30,
                        WuaDisplay::ScrollMode::Loop,
                        WuaDisplay::ScrollDirection::Left);
}

void loop() {
    display.update();   // drives the non-blocking scroll
}
```

## Build

This repository doubles as the library and a build harness, with one PlatformIO
environment per backend:

```sh
pio run -e lmx1               # LMX1 (FastLED)
pio run -e lmx2               # LMX2 (AW20216S)
pio run -e lmx1 -t upload     # build + flash
```

`build_src_filter` excludes the inactive panel so only the selected backend's
dependency is compiled. When consuming this as a library, define the board flag
in your project's `platformio.ini`:

```ini
build_flags = -D WUA_BOARD_LMX1
```

### Dependencies

- `adafruit/Adafruit GFX Library` — both backends
- `fastled/FastLED` — LMX1
- `Wualeds_AW20216S` — LMX2

## Adding a backend

1. Create a panel class deriving from `Adafruit_GFX`.
2. Implement `drawPixel()` and the contract: `begin()`, `clear()`, `flush()`,
   `blur()` (`blur` may be a no-op).
3. Wire it into `src/WuaDisplay_LMX.h` behind a new `WUA_BOARD_*` flag.

No changes to the high-level layer are needed.
