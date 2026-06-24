# 06 — Examples

The bundled sketches in [`examples/`](../examples) all run on **any backend**:
each opens with a small config block that picks the backend and its wiring at
compile time, then drives the **same** high-level API. They were ported from the
upstream [`Wualeds_AW20216S`](https://github.com/WuaLeds/Wualeds_AW20216S)
examples, here going through the two-layer API instead of the chip directly.

> **Reference only.** PlatformIO doesn't compile a library's `examples/` folder
> automatically. To run one, copy its sketch into a project that consumes
> `WuaDisplay_LMX` and build with the desired board flag — or, in this repo:
> ```sh
> cp examples/Basic/Basic.ino src/main.cpp && pio run -e lmx1 -t upload && rm src/main.cpp
> ```

## The shared anatomy of every sketch

1. *(effects examples only)* `#define WUA_ENABLE_EFFECTS`
2. `#include "WuaDisplay_LMX.h"`
3. A `#if defined(WUA_BOARD_*)` block declares `display` with the right
   constructor and (for AW20216S) the SPI pins.
4. `setup()` — start SPI if needed, `display.begin()`, draw the initial frame.
5. `loop()` — animate; call `display.update()` for non-blocking examples.

---

## Catalog

| Example | What it shows | Key API |
| --- | --- | --- |
| [Basic](#basic) | Moving pixel + full-screen flash | `panel().drawPixel`, `fillScreen`, `flush` |
| [TextScroll](#textscroll) | Non-blocking scrolling marquee | `startScroll`, `update` |
| [Brightness](#brightness) | Master brightness ramp | `setBrightness` |
| [ColorWheel](#colorwheel) | Animated HSV sweep | `panel()` per-pixel, `color565` |
| [SpatialSine](#spatialsine) | Plasma from a spatial `sin()` field | `panel()` per-pixel |
| [IconViewer](#iconviewer) | Multi-color icons from ASCII-art maps | `panel().drawPixel` |
| [Pong](#pong) | Minimal Pong animation | GFX primitives |
| [GameOfLife](#gameoflife) | Conway's Life on a cell buffer | `panel().drawPixel` |
| [FirePalette](#firepalette) | Classic fire from a heat buffer | `panel()` + palette |
| [VuMeter](#vumeter) | Bar-graph VU meter | GFX rects, input |
| [Blur](#blur) | Bloom/soften the live buffer | `applyBlur` *(effects)* |
| [Crossfade](#crossfade) | Smooth blend between frames | `createFrame`, `startCrossfade` *(effects)* |

---

## Basic

The "hello world". A single pixel travels every `(x, y)` of the canvas, then the
whole screen flashes blue and clears — repeating forever.

1. Reads the canvas size from `display.panel().width()/height()`, so it is
   **size-agnostic** and runs unchanged on a 7×9 or a 12×12.
2. Lights one pixel with `panel().drawPixel(x, y, color)`, `flush()`es, then
   clears it — a classic "scanner" sweep.
3. `panel().fillScreen(color)` + `flush()` for the flash; `clear()` + `flush()`
   to blank.

**Teaches:** the raw GFX escape hatch, `color565`, and the draw→`flush` cycle.

## TextScroll

A non-blocking horizontal marquee.

1. `startScroll(text, speedMs, mode, dir)` arms the scroll.
2. `loop()` just calls `update()` — the text advances one pixel per interval and
   the CPU stays free for everything else.

**Teaches:** the cooperative-animation pattern (`start* … update()`), the right
way to show text on narrow panels.

## Brightness

Fills the panel with a solid color once, then ramps `setBrightness(0..255)` up
and down forever so the panel "breathes" from black to full.

1. `panel().fillScreen(cyan)` + `flush()` once in `setup()`.
2. A helper ramps the level, calling `setBrightness(level)` then `flush()` each
   step (the `flush()` matters on LMX1, which applies the scale at `show()`).

**Teaches:** hardware master brightness, identical across backends.

## ColorWheel

Sweeps an HSV hue across the matrix and animates it over time, mapping each
pixel's position/phase to a color via `color565` and per-pixel `drawPixel`.

**Teaches:** generating color fields, smooth animation without effects.

## SpatialSine

A plasma-like pattern: each pixel's brightness/color comes from a spatial
`sin()` field evaluated at its `(x, y)` and the current time.

**Teaches:** procedural full-frame rendering through the GFX escape hatch.

## IconViewer

Draws multi-color icons defined as ASCII-art maps, translating each character to
a color and plotting it with `drawPixel`.

**Teaches:** turning compact human-readable art into pixels; good template for
sprite/glyph tables.

## Pong

A minimal Pong animation built from GFX primitives (rectangles for paddles/ball)
with simple bounce logic.

**Teaches:** GFX shape drawing and a basic animation/update loop.

## GameOfLife

Conway's Game of Life on a software cell buffer; each generation is computed in
RAM, then rendered to the panel pixel by pixel.

**Teaches:** keeping your own model buffer and presenting it through the panel.

## FirePalette

The classic doom-fire effect: a software heat buffer is advected/cooled each
frame and mapped through a fire palette to the panel.

**Teaches:** software-buffer effects that look identical on every backend
(because the buffer, not the hardware, holds the state).

## VuMeter

A bar-graph VU meter driven by Serial- or analog input, drawing level bars with
GFX rectangles.

**Teaches:** turning live input into a display; mapping a value to bar height.

## Blur *(effects build)*

Draws something, then calls `applyBlur(amount)` to bloom/soften the live buffer
before `flush()`.

1. Requires `WUA_ENABLE_EFFECTS`.
2. On LMX1/LMX1p it blurs the FastLED array; on LMX2/LMX2d it blurs the RAM
   shadow buffer — same look.

**Teaches:** the one-call live blur.

## Crossfade *(effects build)*

The showcase for the effects engine. It builds **five** very different frames —
a letter with/without background, a heart bitmap with/without background, and a
plain color — then crossfades through them forever.

1. `#define WUA_ENABLE_EFFECTS` before the include (the sketch defines it itself,
   so it compiles under any env).
2. `createFrame()` allocates each `Frame` pre-sized to the panel.
3. Each frame is composed with the **same** calls used on the panel —
   `display.printAligned(*frame, ...)`, `display.setTextColorRGB(*frame, ...)`,
   and raw `frame->fillScreen / drawBitmap`.
4. `startCrossfade(*frames[current], *frames[next], FADE_MS)` blends; `loop()`
   calls `update()` and queues the next transition when `!isCrossfading()`.

**Teaches:** off-screen composition, the unified panel/frame drawing calls, and
non-blocking transitions that are byte-for-byte identical across backends.

➡️ Back to the [index](README.md).
