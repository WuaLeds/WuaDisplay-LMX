---
name: wuadisplay-lmx
description: >-
  Expert assistant for the WuaDisplay-LMX Arduino/ESP32 library: a two-layer,
  high-level display API (text, scroll, brightness, blur, crossfade) over four
  LED backends (LMX1, LMX2, LMX2d, LMX1p) selected by a compile-time build flag.
  Use when a user is writing, building, wiring, or debugging firmware that uses
  `WuaDisplay`, `WuaDisplay_LMX.h`, or any LMX* panel.
metadata:
  type: reference
  language: cpp
  framework: arduino-platformio-esp32
---

# Skill: WuaDisplay-LMX

You are helping with **WuaDisplay-LMX**, a PlatformIO Arduino (ESP32) library
that exposes one high-level display API on top of several LED panel backends,
selected at **compile time**. This file is self-contained: use it to answer
questions and generate code without needing the repo open.

## Mental model (read first)

- **Two layers, static polymorphism.** Layer 2 `WuaDisplayLMX<Panel>` (header-
  only template) implements text/scroll/color/brightness/effects once. Layer 1
  panels (`LMX1/LMX2/LMX2D/LMX1P`) are `Adafruit_GFX` subclasses that own the
  physical layout and push pixels. No virtual dispatch, no heap on the hot path.
- **`WuaDisplay`** is a compile-time alias chosen by a `-D WUA_BOARD_*` flag.
  The user's sketch is backend-agnostic **except the constructor line**.
- **It's a library, not an app.** `src/` has no `main`; you compile a sketch
  against it. In-repo, CI copies an example to `src/main.cpp` and runs `pio run`.

## Backends (pick ONE flag)

| Flag | Class | Canvas | Driver / dep | Constructor |
| --- | --- | --- | --- | --- |
| `WUA_BOARD_LMX1` *(default)* | `LMX1` | `(7·N)×9` | FastLED (SK6812) | `WuaDisplay display(N);` |
| `WUA_BOARD_LMX2` | `LMX2` | `6×12` | Wualeds_AW20216S (SPI) | `WuaDisplay display(csPin);` |
| `WUA_BOARD_LMX2D` | `LMX2D` | `12×12` | Wualeds_AW20216S (SPI, 2 CS) | `WuaDisplay display(csLeft, csRight);` |
| `WUA_BOARD_LMX1P` | `LMX1P` | `18×7` | FastLED (SK6812) | `WuaDisplay display;` |

- **LMX1:** N modules (7×9) chained, ordered right-to-left.
- **LMX2:** single AW20216S; keeps a RAM shadow buffer so blur works.
- **LMX2d:** one PCB, two AW20216S; left chip = cols [0,6), right = cols [6,12).
  Keeps a RAM shadow buffer too, so blur works (bloom crosses the chip seam).
- **LMX1p:** one PCB, two LMX1 modules as one chain; right-to-left **vertical
  serpentine**: `column=(W-1)-x; offset=(column&1)?(H-1-y):y; index=column*H+offset`.

## Panel contract (to add a backend)

An `Adafruit_GFX` subclass implementing: `begin()`, `clear()`, `flush()`,
`blur(uint8_t)`, `setBrightness(uint8_t)`, and `drawPixel(x,y,color)`. Then add
an `#if/#elif` branch in `WuaDisplay_LMX.h` and guard the new `.cpp` with the
board flag.

## High-level API (`WuaDisplay`)

```cpp
// lifecycle
WuaDisplay display(args...);          // args forwarded to the panel
void begin();                          // init hw; call once in setup()
void clear();  void flush();           // clear buffer / push to hardware
Panel &panel();                        // raw Adafruit_GFX escape hatch
void update();                         // call EVERY loop(); advances scroll+crossfade

// color
static uint16_t color565(r,g,b);
void setTextColorRGB(r,g,b);

// brightness (0..255 master, hardware level)
void setBrightness(uint8_t level);

// aligned text — HAlign::{Left,Center,Right}
void printAligned(text, align=Center, y=1);
void printAlignedRefresh(text, align=Center, y=1);   // clear+draw+flush

// non-blocking scroll — ScrollMode::{Loop,Once}, ScrollDirection::{Left,Right}
void startScroll(text, speedMs=30, mode=Once, dir=Left, y=1);
void stopScroll();  bool isScrolling();  void setScrollCallback(void(*)());

// EFFECTS — only if WUA_ENABLE_EFFECTS is defined before the include
void applyBlur(uint8_t amount);
using Frame = GFXcanvas16;  Frame *createFrame();        // caller deletes it
void printAligned(target, text, align, y);               // draw into a Frame
void setTextColorRGB(target, r, g, b);
void startCrossfade(from, to, durationMs, stepMs=30);
bool isCrossfading();  void stopCrossfade();
```

## Canonical recipes

**Minimal scroll (any backend):**
```cpp
#include "WuaDisplay_LMX.h"
WuaDisplay display(1);
void setup(){ display.begin(); display.setTextColorRGB(0,180,255);
  display.startScroll("HELLO", 30, WuaDisplay::ScrollMode::Loop); }
void loop(){ display.update(); }
```

**AW20216S backends need SPI started before begin():**
```cpp
#include <SPI.h>
SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, CS_PIN);   // LMX2
// or  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);     // LMX2d (CS per chip)
display.begin();
```

**Crossfade (effects build):**
```cpp
#define WUA_ENABLE_EFFECTS
#include "WuaDisplay_LMX.h"
WuaDisplay display(1);
WuaDisplay::Frame *a, *b;
void setup(){ display.begin(); a=display.createFrame(); b=display.createFrame();
  a->fillScreen(0); display.printAligned(*a,"A");
  b->fillScreen(WuaDisplay::color565(120,0,160)); display.printAligned(*b,"B");
  display.startCrossfade(*a,*b,600); }
void loop(){ display.update(); }
```

## Build (PlatformIO)

Envs in `platformio.ini`: `lmx1`, `lmx2`, `lmx2d`, `lmx1p`. Each sets its
`-D WUA_BOARD_*` and a mutually-exclusive `build_src_filter`, and pulls only its
own dependency (FastLED or Wualeds_AW20216S; Adafruit GFX always).

```sh
cp examples/<Name>/<Name>.ino src/main.cpp && pio run -e <env> && rm src/main.cpp
```

To enable effects, add `-D WUA_ENABLE_EFFECTS` (or `#define` it before the
include in the sketch).

## Overridable geometry macros (build flags)

`LMX1_MODULE_WIDTH`(7) `LMX1_MODULE_HEIGHT`(9) `LMX1_LED_PIN`(5) ·
`LMX2_WIDTH`(6) `LMX2_HEIGHT`(12) · `LMX2D_WIDTH`(12) `LMX2D_HEIGHT`(12) ·
`LMX1P_WIDTH`(18) `LMX1P_HEIGHT`(7) `LMX1P_LED_PIN`(5).

## Gotchas / things to get right

- **Always call `display.update()` in `loop()`** for scroll/crossfade — they are
  cooperative and never block.
- **`clear()`/`printAligned()` don't push.** Call `flush()` (or use
  `printAlignedRefresh`). Scroll/crossfade flush themselves.
- **AW20216S framebuffer is write-only.** Blur on LMX2 and LMX2d works via a RAM
  shadow buffer that `flush()` pushes to the chip(s); blur is real on all four
  backends. Software-buffer effects (FirePalette) look identical across backends
  by design.
- **FastLED data pin is compile-time** (`LMX1_LED_PIN`/`LMX1P_LED_PIN`), not a
  constructor argument.
- **LMX1p takes no constructor args** (fixed 18×7): `WuaDisplay display;`.
- **Frames are owned by the caller** — `delete` them (usually one-off in setup);
  they must match the panel size or `startCrossfade` ignores them.
- **Narrow panels**: static text is barely legible at 6–7 px wide; prefer
  scrolling.
- **Effects API only exists under `WUA_ENABLE_EFFECTS`** — referencing `Frame`/
  `startCrossfade` without it is a compile error.
- **All repo text (code, comments, docs, commits, PRs) must be in English.**

## File map

- `src/WuaDisplayLMX.h` — Layer 2 engine (template) + Panel contract docs.
- `src/WuaDisplay_LMX.h` — umbrella header; compile-time backend selection.
- `src/LMX1.{h,cpp}` `LMX2.{h,cpp}` `LMX2D.{h,cpp}` `LMX1P.{h,cpp}` — backends.
- `examples/` — reference sketches (Basic, TextScroll, Brightness, ColorWheel,
  SpatialSine, IconViewer, Pong, GameOfLife, FirePalette, VuMeter, Blur,
  Crossfade).
- `docs/` — full human documentation (overview, getting-started, architecture,
  backends, API, examples).
