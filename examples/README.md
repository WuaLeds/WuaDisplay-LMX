# Examples

Example sketches written against the high-level `WuaDisplay` API of this
library. Each sketch runs on **either backend**, selected at build time:

- `-D WUA_BOARD_LMX1` — one or more 7×9 SK6812 RGB modules driven over a single
  data pin (FastLED). This is also the **default** build when no flag is set.
- `-D WUA_BOARD_LMX2` — a single 6×12 AW20216S RGB matrix driven over SPI.
- `-D WUA_BOARD_LMX2D` — one PCB with two AW20216S chips side by side (one SPI
  bus, one chip-select each), exposed as a single 12×12 canvas.

> **About the sketches below.** Their per-board config blocks select between the
> LMX1 and LMX2 backends only. The `LMX2D` backend shares the exact same
> high-level API; to run a sketch on it, declare the display with the two
> chip-select pins (`WuaDisplay display(CS_LEFT, CS_RIGHT);`) and start the SPI
> bus, as shown under *LMX2d wiring* below. CI compile-checks LMX2d from a
> minimal sketch rather than from these examples.

They were ported from the upstream
[`Wualeds_AW20216S`](https://github.com/WuaLeds/Wualeds_AW20216S) examples,
which talk to the chip directly. Here they go through the two-layer API instead:
high-level helpers (`color565`, `setTextColorRGB`, `startScroll`, `clear`,
`flush`) plus the raw `Adafruit_GFX` escape hatch exposed by `display.panel()`
for per-pixel drawing.

> **Reference only.** PlatformIO does not compile a library's `examples/`
> folder automatically. To run one, copy its sketch into a project that
> consumes `WuaDisplay_LMX` and builds with the desired board flag.

## Board selection

The configuration block at the top of every sketch picks the backend and its
wiring at compile time:

```cpp
#if defined(WUA_BOARD_LMX2)
  // single 6×12 AW20216S over SPI
  WuaDisplay display(CS_PIN);
#else
  // LMX1 (also the default when no board flag is set): the constructor argument
  // is the number of chained 7×9 SK6812 modules — 1 for a single module, N for N.
  WuaDisplay display(1);
#endif
```

### LMX2 wiring (SPI)

The pins below are **placeholders** for an ESP32-C3 — adjust them to your
actual board/wiring:

```cpp
// ---- Wiring (placeholder values for ESP32-C3 — adjust to your board) ----
#define PIN_SCK  6
#define PIN_MISO 5
#define PIN_MOSI 7
#define CS_PIN   10
```

The SPI bus is started explicitly before `display.begin()`, because
`LMX2::begin()` only initialises the chip and does not configure custom SPI
pins:

```cpp
SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, CS_PIN);
WuaDisplay display(CS_PIN);
// ...
display.begin();
```

### LMX2d wiring (SPI, two chip-selects)

The LMX2d is one PCB with two AW20216S chips sharing a single SPI bus, each with
its own chip-select. Start the bus once and pass both CS pins to the display
(left chip → columns 0–5, right chip → columns 6–11 of the 12×12 canvas):

```cpp
#define PIN_SCK   6
#define PIN_MISO  5
#define PIN_MOSI  7
#define CS_LEFT   10   // chip driving the left 6×12 half
#define CS_RIGHT  1    // chip driving the right 6×12 half

SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);
WuaDisplay display(CS_LEFT, CS_RIGHT);
// ...
display.begin();
```

### LMX1 wiring (single data pin)

LMX1 modules are addressable SK6812 LEDs driven through FastLED on one data
pin (`LMX1_LED_PIN`, default 5). No SPI setup is needed; just construct the
display with the number of chained modules and call `begin()`:

```cpp
WuaDisplay display(1); // one 7×9 module; pass N for N chained modules
// ...
display.begin();
```

## Examples

| Example      | What it shows                                                  |
| ------------ | -------------------------------------------------------------- |
| Basic        | Moving pixel + full-screen flash (`drawPixel`, `fillScreen`).  |
| ColorWheel   | Animated HSV color sweep across the matrix.                    |
| TextScroll   | Non-blocking scrolling text via `startScroll`.                 |
| IconViewer   | Multi-color icons drawn pixel-by-pixel from ASCII-art maps.    |
| SpatialSine  | Plasma-like pattern from a spatial `sin()` field.              |
| Pong         | Minimal Pong animation using GFX primitives.                   |
| GameOfLife   | Conway's Game of Life on a software cell buffer.               |
| FirePalette  | Classic fire effect rendered from a software heat buffer.      |
| VuMeter      | Bar-graph VU meter (Serial- or analog-driven input).          |
| Crossfade    | Non-blocking blend between frames (opt-in effects engine).     |
| Blur         | Bloom/soften the live buffer with `applyBlur` (LMX1 + LMX2).   |
| Brightness   | Master brightness ramp via `setBrightness` (LMX1 + LMX2).      |

## Not ported

The following upstream examples are intentionally **not** included, because
they depend on AW20216S hardware features that the high-level `WuaDisplay` API
does not expose:

| Upstream example   | Reason                                                       |
| ------------------ | ------------------------------------------------------------ |
| BrightnessFade     | Uses `setGlobalCurrent()` (chip master brightness).          |
| WhiteBalance       | Uses per-channel `setScaling()`.                             |
| breathing          | Hardware auto-breath (per-LED PWM engine).                   |
| MixedBreathing     | Hardware auto-breath (per-LED PWM engine).                   |
| PWMFrequencySweep  | Writes the PWM-frequency control register.                   |
| RegisterDump       | Reads/writes raw chip registers for diagnostics.             |
| MultiPanel         | Drives several AW20216S chips; LMX2 is single-panel by design (use the LMX1 backend for chained modules). |

## Caveats

- Both panels are **narrow** (LMX2 is 6 px wide, one LMX1 module 7 px), so
  static text is barely legible; `TextScroll` uses horizontal scrolling, which
  is what makes sense here. Chain several LMX1 modules for a wider marquee.
- `applyBlur()` works on **all** backends: LMX1 blurs its FastLED LED array,
  while LMX2 and LMX2d blur a small RAM shadow buffer in software (the AW20216S
  framebuffer itself cannot be read back). On LMX2d the bloom crosses the seam
  between the two chips, so it looks the same as on a single continuous panel.
