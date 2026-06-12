# LMX2 examples

Example sketches for the **LMX2** backend (AW20216S 6×12 RGB matrix, SPI),
written against the high-level `WuaDisplay` API of this library.

They were ported from the upstream
[`Wualeds_AW20216S`](https://github.com/WuaLeds/Wualeds_AW20216S) examples,
which talk to the chip directly. Here they go through the two-layer API instead:
high-level helpers (`color565`, `printAligned`, `startScroll`, `clear`,
`flush`) plus the raw `Adafruit_GFX` escape hatch exposed by `display.panel()`
for per-pixel drawing.

> **Reference only.** PlatformIO does not compile a library's `examples/`
> folder automatically. To run one, copy its sketch into a project that
> consumes `WuaDisplay_LMX` and builds with `-D WUA_BOARD_LMX2`.

## Wiring

Every sketch starts from the same configuration block. The pins below are
**placeholders** for an ESP32-C3 — adjust them to your actual board/wiring:

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

## Examples

| Example      | What it shows                                                  |
| ------------ | -------------------------------------------------------------- |
| Basic        | Moving pixel + full-screen flash (`drawPixel`, `fillScreen`).  |
| ColorWheel   | Animated HSV color sweep across the matrix.                    |
| TextScroll   | Non-blocking scrolling text via `startScroll`.                 |
| IconViewer   | Drawing monochrome bitmaps with `drawBitmap`.                  |
| SpatialSine  | Plasma-like pattern from a spatial `sin()` field.              |
| Pong         | Minimal Pong animation using GFX primitives.                   |
| GameOfLife   | Conway's Game of Life on a software cell buffer.               |
| FirePalette  | Classic fire effect rendered from a software heat buffer.      |
| VuMeter      | Bar-graph VU meter (simulated audio input).                    |

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

- The LMX2 matrix is only **6 px wide**, so static text is barely legible;
  `TextScroll` uses horizontal scrolling, which is what makes sense here.
- `applyBlur()` is a **no-op** on LMX2 (the AW20216S framebuffer cannot be read
  back), so effects such as `FirePalette` keep their own software buffer.
