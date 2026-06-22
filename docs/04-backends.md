# 04 — Backends & hardware

Every component compatible with the library, and how it is built. There are
**four** backends grouped into two hardware families:

| Backend | Canvas | Driver | Family | Constructor |
| --- | --- | --- | --- | --- |
| [LMX1](#lmx1) | `(7·N) × 9` | FastLED | SK6812 addressable | `WuaDisplay display(N);` |
| [LMX2](#lmx2) | `6 × 12` | AW20216S (SPI) | AW20216S matrix | `WuaDisplay display(csPin);` |
| [LMX2d](#lmx2d) | `12 × 12` | AW20216S ×2 (SPI) | AW20216S matrix | `WuaDisplay display(csLeft, csRight);` |
| [LMX1p](#lmx1p) | `18 × 7` | FastLED | SK6812 addressable | `WuaDisplay display;` |

All four are `Adafruit_GFX` subclasses satisfying the
[Panel contract](03-architecture.md#the-panel-contract).

---

## The two hardware families

### SK6812 addressable LEDs (via FastLED) — *LMX1, LMX1p*

Each LED is an individually-addressable RGB device on a single serial data line.
The whole panel is **one 1-D chain**; the order in which you write the `CRGB[]`
array is the order the data clocks down the wire. Geometry is therefore a
*mapping problem*: the backend converts logical `(x, y)` into the right index in
that chain.

- **Driver:** [FastLED](https://github.com/FastLED/FastLED).
- **Pins:** one data pin (compile-time constant, because FastLED templates on
  it).
- **Color:** the backend converts GFX RGB565 → `CRGB` RGB888.
- **Brightness:** `FastLED.setBrightness()` — a global output scale applied at
  `show()` time (so it lands on the next `flush()`).
- **Blur:** real, via FastLED `blur2d` over the LED array (an `fl::XYMap`
  describes the physical layout to the blur).
- **Power note:** addressable LEDs at full white draw substantial current; size
  your 5V supply accordingly. The backend caps FastLED at 5V/500mA by default.

### AW20216S matrix (via SPI) — *LMX2, LMX2d*

The AW20216S is a dedicated RGB LED matrix driver addressed over **SPI** with a
per-chip **chip-select**. You write pixels with `setPixel(x, y, r, g, b)` and
push with `show()`.

- **Driver:** [`Wualeds_AW20216S`](https://github.com/WuaLeds/Wualeds_AW20216S).
- **Pins:** an SPI bus (`SCK`/`MISO`/`MOSI`) plus one **CS** per chip. Start the
  bus with `SPI.begin(...)` **before** `display.begin()`.
- **Color:** the backend unpacks GFX RGB565 → 8-bit-per-channel.
- **Brightness:** the chip's **global current** register (master brightness),
  written immediately; `begin()` seeds it at `0x40`.
- **Framebuffer is write-only:** you cannot read pixels back from the chip. That
  shapes the effects design (see LMX2's RAM shadow buffer below).

---

## LMX1

**N WuaDisplay modules (7×9 px each) chained horizontally → a `(7·N) × 9`
canvas.** Files: [`src/LMX1.h`](../src/LMX1.h), [`src/LMX1.cpp`](../src/LMX1.cpp).

### Physical layout

A single module is a 7-wide × 9-tall block of SK6812 LEDs. Modules are chained
and ordered **right-to-left** along the data line. The backend derives the
module count from the canvas width, maps `(x, y)` to the right module, and
indexes within it — so the same mapping is valid for any `N`.

### Construction & wiring

```cpp
WuaDisplay display(7);          // 7 chained modules → 49 × 9
// DIN → LMX1_LED_PIN (default GPIO 5); 5V, GND.
display.begin();
```

### Overridable geometry (build flags)

| Macro | Default | Meaning |
| --- | --- | --- |
| `LMX1_MODULE_WIDTH` | `7` | px per module, horizontal |
| `LMX1_MODULE_HEIGHT` | `9` | px per module, vertical |
| `LMX1_LED_PIN` | `5` | FastLED data pin |

### Notes

- The framebuffer is a heap `CRGB[]` sized from the runtime module count.
- `blur()` is fully supported (FastLED `blur2d` + an `fl::XYMap`).
- It is the **default** backend: active whenever no other board flag is set.

---

## LMX2

**A single 6×12 AW20216S RGB matrix over SPI.** Files:
[`src/LMX2.h`](../src/LMX2.h), [`src/LMX2.cpp`](../src/LMX2.cpp).

### Construction & wiring

```cpp
#include <SPI.h>
SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, CS_PIN);
WuaDisplay display(CS_PIN);     // AW20216S on its chip-select
display.begin();                // seeds global current + full scaling
```

### The RAM shadow buffer (why blur works)

The AW20216S framebuffer can't be read back, so the backend keeps a small
**RGB888 RAM mirror** (`6·12·3` bytes). `drawPixel()` writes the mirror, `blur()`
averages it in place (neighbour averaging), and `flush()` pushes it to the chip.
That is how `applyBlur()` produces a real bloom on a write-only device.

### Overridable geometry

| Macro | Default |
| --- | --- |
| `LMX2_WIDTH` | `6` |
| `LMX2_HEIGHT` | `12` |

### Notes

- At 6 px wide, static text is barely legible — this panel shines for pixel art
  and effects. Use scrolling for text.
- `setBrightness()` maps to the chip's global-current register.

---

## LMX2d

**One PCB carrying *two* AW20216S chips side by side → a single 12×12 canvas.**
Files: [`src/LMX2D.h`](../src/LMX2D.h), [`src/LMX2D.cpp`](../src/LMX2D.cpp).

### Physical layout

It is **not** two chained LMX2 boards: it is one PCB with two chips on a shared
SPI bus, each with its own chip-select. The two 6×12 halves combine into 12×12:

- left chip → columns `[0, 6)`
- right chip → columns `[6, 12)`

`drawPixel()` routes each pixel to the chip that owns its column, in that chip's
local coordinates. `begin()` initialises both chips independently (and reports,
but tolerates, a half that doesn't answer, so one bad chip won't hard-lock the
firmware). `clear()` / `flush()` / `setBrightness()` fan out to both.

### Construction & wiring

```cpp
#include <SPI.h>
SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);     // shared bus; each chip has its own CS
WuaDisplay display(CS_LEFT, CS_RIGHT);      // two chip-selects, one 12×12 canvas
display.begin();
```

### Overridable geometry

| Macro | Default | Meaning |
| --- | --- | --- |
| `LMX2D_WIDTH` | `2 * LMX2_WIDTH` (12) | full canvas width |
| `LMX2D_HEIGHT` | `LMX2_HEIGHT` (12) | full canvas height |

### Notes

- `blur()` is a no-op on LMX2d (matching the write-only AW20216S framebuffer);
  effects keep their own software buffer.

---

## LMX1p

**The "panoramic" panel: one PCB with *two* LMX1 modules wired as a single
continuous SK6812 chain → an 18×7 canvas, used in landscape.** Files:
[`src/LMX1P.h`](../src/LMX1P.h), [`src/LMX1P.cpp`](../src/LMX1P.cpp).

### Physical layout — the serpentine

Each LMX1 module is 7×9 in portrait; rotated 90° for landscape it measures 9×7,
so two side by side form **18×7** (126 LEDs). It is **one continuous chain**, not
two independent modules, and the chain runs as a **vertical serpentine, column
by column from RIGHT to LEFT**:

1. the first LED is the **top-right** pixel,
2. the chain runs **down** that rightmost column,
3. hops to the next column on its left and climbs back **up**,
4. …and so on (boustrophedon).

The mapping from logical `(x, y)`:

```
column = (WIDTH - 1) - x                          // 0 = rightmost column
offset = (column odd) ? (HEIGHT - 1 - y) : y      // climb up on odd columns
index  = column * HEIGHT + offset
```

### Construction & wiring

```cpp
WuaDisplay display;             // fixed 18 × 7 — no module count
// DIN → LMX1P_LED_PIN (default GPIO 5, shared with LMX1); 5V, GND.
display.begin();
```

### Overridable geometry

| Macro | Default | Meaning |
| --- | --- | --- |
| `LMX1P_WIDTH` | `2 * LMX1_MODULE_HEIGHT` (18) | full canvas width |
| `LMX1P_HEIGHT` | `LMX1_MODULE_WIDTH` (7) | full canvas height |
| `LMX1P_LED_PIN` | `LMX1_LED_PIN` (5) | FastLED data pin |

### Notes

- The framebuffer is a fixed `CRGB[126]` member (geometry is compile-time
  constant), so there is no heap allocation.
- Driven through FastLED exactly like LMX1.

---

## Choosing a backend

| If you want… | Use |
| --- | --- |
| A wide marquee from cheap chained modules | **LMX1** (chain N) |
| A compact square-ish RGB matrix | **LMX2** |
| A larger 12×12 square on one tidy PCB | **LMX2d** |
| A wide, short landscape banner on one PCB | **LMX1p** |

➡️ Next: [05 — API reference](05-api-reference.md).
