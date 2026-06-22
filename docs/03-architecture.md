# 03 — Architecture

WuaDisplay-LMX is organised in **two layers** joined by **static (compile-time)
polymorphism**. The result: one high-level API written once, running on any
backend with no runtime cost.

```
        ┌─────────────────────────────────────────────────────────┐
        │  Your sketch                                              │
        │     WuaDisplay display(...);  display.startScroll(...)    │
        └───────────────────────────┬─────────────────────────────┘
                                     │  (compile-time alias)
        ┌───────────────────────────▼─────────────────────────────┐
        │  Layer 2 — WuaDisplayLMX<Panel>   (header-only template)  │
        │     text · alignment · non-blocking scroll · color ·      │
        │     brightness · effects (blur, crossfade)                │
        └───────────────────────────┬─────────────────────────────┘
                                     │  calls the Panel contract
        ┌───────────────────────────▼─────────────────────────────┐
        │  Layer 1 — pixel panels (Adafruit_GFX subclasses)         │
        │     LMX1 · LMX2 · LMX2d · LMX1p                            │
        │     own the physical layout + push pixels to the chip     │
        └───────────────────────────┬─────────────────────────────┘
                                     │
        ┌───────────────────────────▼─────────────────────────────┐
        │  Hardware drivers:  FastLED (SK6812)  ·  AW20216S (SPI)   │
        └──────────────────────────────────────────────────────────┘
```

## Layer 2 — the high-level engine

[`WuaDisplayLMX<Panel>`](../src/WuaDisplayLMX.h) is a **header-only class
template**. It implements everything that is independent of the hardware:

- aligned text (`printAligned`, `printAlignedRefresh`)
- non-blocking horizontal scroll (`startScroll` / `update`)
- color helpers (`color565`, `setTextColorRGB`)
- master brightness pass-through (`setBrightness`)
- the opt-in effects engine (`applyBlur`, `Frame`, `startCrossfade`)

It **composes the panel by value** (`Panel _panel;`) — no pointer, no heap, no
virtual dispatch. Every call like `display.startScroll(...)` ends in a direct,
inlinable call into the concrete panel's methods.

## Layer 1 — pixel panels

Each backend is a small class that derives from `Adafruit_GFX` and implements
the single pixel primitive `drawPixel()` plus the **Panel contract**. The panel
*owns the physical layout*: it translates clean logical `(x, y)` into wherever
that pixel actually lives in the hardware (a serpentine LED index, a chip + local
coordinate, etc.).

Because each panel is an `Adafruit_GFX`, you also get the entire GFX drawing
suite for free (`drawLine`, `drawBitmap`, `print`, fonts, `fillScreen`, ...),
both from Layer 2 and directly via `display.panel()`.

## The Panel contract

This is the seam between the two layers. Any class that satisfies it can be
driven by Layer 2:

| Member | Meaning |
| --- | --- |
| *derives from* `Adafruit_GFX` | provides `drawPixel()`, `print()`, `setCursor()`, `getTextBounds()`, `setTextColor()`, `setTextWrap()`, `width()`, `height()` |
| `void begin()` | initialise the hardware |
| `void clear()` | clear the framebuffer (native/fast when possible) |
| `void flush()` | push the framebuffer to the hardware in one burst |
| `void blur(uint8_t)` | buffer-level blur (no-op when unsupported) |
| `void setBrightness(uint8_t)` | master brightness 0..255 at the hardware level |
| `void drawPixel(int16_t, int16_t, uint16_t)` | the one pixel primitive everything builds on |

The contract is **enforced by use**: if a panel is missing a member, the
template simply fails to compile. There is no abstract base class and therefore
no runtime cost.

## Two axes of variation, two mechanisms

| Axis | When it's resolved | How |
| --- | --- | --- |
| **Which backend** (LMX1 / LMX2 / LMX2d / LMX1p) | **Compile time** | a `-D WUA_BOARD_*` flag picks the `Panel` type in `WuaDisplay_LMX.h` |
| **Hardware setting** (module count, CS pins, ...) | **Run time** | forwarded to the panel constructor (`WuaDisplay display(7);`) |

### Compile-time backend selection

[`WuaDisplay_LMX.h`](../src/WuaDisplay_LMX.h) is the public umbrella header. It
picks the concrete panel from the board flag and exposes the alias `WuaDisplay`:

```cpp
#if defined(WUA_BOARD_LMX2D)
  using WuaDisplay = WuaDisplayLMX<LMX2D>;
#elif defined(WUA_BOARD_LMX2)
  using WuaDisplay = WuaDisplayLMX<LMX2>;
#elif defined(WUA_BOARD_LMX1P)
  using WuaDisplay = WuaDisplayLMX<LMX1P>;
#else
  using WuaDisplay = WuaDisplayLMX<LMX1>;   // default
#endif
```

### One backend, one dependency — guaranteed

Each backend's translation unit is wrapped in its own board-flag guard, so a
consumer compiles only the active one **regardless of `build_src_filter`**:

- `LMX1.cpp` compiles in the LMX1/default case (no LMX2/LMX2d/LMX1p flag set) →
  pulls **FastLED**.
- `LMX2.cpp` only under `WUA_BOARD_LMX2` → pulls **AW20216S**.
- `LMX2D.cpp` only under `WUA_BOARD_LMX2D` → pulls **AW20216S**.
- `LMX1P.cpp` only under `WUA_BOARD_LMX1P` → pulls **FastLED**.

This matters for downstream projects: selecting the LMX2 backend never drags
FastLED into your build, and vice-versa. Keep these guards consistent with the
`#if/#elif/#else` in `WuaDisplay_LMX.h`.

## The effects engine (opt-in)

The blur + crossfade engine lives in Layer 2 and is gated by
`WUA_ENABLE_EFFECTS`. When the flag is absent, the effect members, frame
buffers, and helper functions are `#ifdef`'d out entirely — the engine is free
of charge if you don't use it.

- **`Frame`** is an alias for `GFXcanvas16` — a full off-screen GFX canvas. You
  compose it with the exact same calls you use on the live panel; the library
  owns the blend, the content is yours.
- **`startCrossfade(a, b, ms)`** blends frame `a → b` over time, advanced from
  `update()`. The blend reads the two frame buffers, interpolates each RGB565
  pixel, and writes the result through `drawPixel()` — which is why it works on
  every backend without the panels knowing anything about it.
- **`applyBlur(amount)`** delegates to the panel's `blur()`, which is where the
  hardware difference is handled (FastLED `blur2d` vs. AW20216S RAM shadow).

## Source layout

| File | Role |
| --- | --- |
| [`src/WuaDisplayLMX.h`](../src/WuaDisplayLMX.h) | Layer 2 engine (template) + the Panel contract docs |
| [`src/LMX1.{h,cpp}`](../src/) | LMX1 backend (SK6812 chain via FastLED) |
| [`src/LMX2.{h,cpp}`](../src/) | LMX2 backend (AW20216S via SPI) |
| [`src/LMX2D.{h,cpp}`](../src/) | LMX2d backend (two AW20216S → 12×12) |
| [`src/LMX1P.{h,cpp}`](../src/) | LMX1p backend (two LMX1 → 18×7 serpentine) |
| [`src/WuaDisplay_LMX.h`](../src/WuaDisplay_LMX.h) | public umbrella header; compile-time backend selection |
| [`examples/`](../examples) | reference sketches (the library ships **no** app entry point) |

## Why static polymorphism (and not virtual)?

These are microcontrollers. A vtable per frame, indirect calls in the
pixel-pushing inner loop, and heap fragmentation are real costs. By resolving
the backend at compile time:

- the compiler inlines `drawPixel()` into the text/scroll/blend loops,
- there is no vtable pointer in the panel object,
- the core engine allocates nothing on the heap (only the optional effects
  `Frame`s do, and only when you ask),

…so you get the ergonomics of a polymorphic API with the footprint of
hand-written, hardware-specific code.

➡️ Next: [04 — Backends & hardware](04-backends.md).
