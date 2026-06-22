# 01 — Why WuaDisplay-LMX

> **The short pitch:** stop rewriting text, scrolling, and effects every time
> the LED hardware changes. Write your UI once against one clean API, and ship
> it on any Wualeds panel by flipping a single build flag.

## The pain it removes

Driving small RGB LED displays is deceptively annoying. Each panel speaks a
different dialect:

- **SK6812 / WS2812B** addressable strips are driven through **FastLED**, as a
  1-D array of LEDs that you have to map to a 2-D grid yourself — and the
  physical wiring is rarely a clean left-to-right, top-to-bottom grid (it
  serpentines, it runs right-to-left, modules are chained...).
- The **AW20216S** is an SPI matrix chip with its own `setPixel` / `show`
  register-level API, an 8-bit-per-channel color model, a global-current
  brightness register, and a framebuffer you **cannot read back**.

If you write straight against the hardware, every product decision leaks into
your application code:

- Want to render text? You re-implement glyph layout per chip.
- Want a scrolling marquee? You re-implement non-blocking timing per chip.
- Want a blur or a crossfade? You discover the AW20216S can't read its own
  buffer and you bolt on a shadow buffer by hand.
- Want to swap a 6×12 matrix for a chain of 7×9 modules? You rewrite the
  coordinate mapping and half your sketch.

Every one of those is solved **once** here, in a layer that doesn't care which
chip is underneath.

## What you get instead

**One API, any panel.** Your sketch talks to a single `WuaDisplay` object:
aligned text, non-blocking horizontal scroll, color helpers, master brightness,
and an opt-in effects engine (blur + crossfade). The concrete hardware is a
compile-time choice — change `-D WUA_BOARD_*` and rebuild; **your code does not
change.**

**Zero-overhead abstraction.** The backend is selected with *static*
polymorphism (a C++ template), not virtual functions. There is **no vtable on
the hot path and no heap allocation** for the core engine. You pay nothing at
runtime for the flexibility — it compiles down to a direct call into the chosen
panel.

**Built on Adafruit_GFX.** Every panel is an `Adafruit_GFX` canvas, so the
entire GFX ecosystem just works: fonts, `print()`, `drawLine`, `drawBitmap`,
`fillScreen`, custom fonts, and so on. WuaDisplay adds the high-level
conveniences on top; the raw GFX escape hatch (`display.panel()`) is always
there when you need it.

**Correct hardware mapping, hidden.** The library owns the physical layout of
each board: the right-to-left chaining of LMX1 modules, the two-chip column
split of LMX2d, the right-to-left vertical serpentine of the LMX1p panoramic
panel. You draw in clean logical `(x, y)`; the backend puts the photons in the
right place.

**Effects that survive a write-only framebuffer.** `applyBlur()` works on both
families: FastLED panels blur their LED array directly; AW20216S panels blur a
small RAM shadow buffer in software. The crossfade engine composes off-screen
`Frame`s and blends them through `drawPixel()`, so it is backend-agnostic by
construction — the **same** crossfade sketch runs on LMX1 and LMX2 unchanged.

## Who it's for

- **Product firmware** that needs to run on more than one panel variant from a
  single codebase. WuaDisplay-LMX is the consolidated base library other
  Wualeds projects depend on — e.g. `WuaDisplay-r1-firmware`, which chains 7
  LMX1 modules into one large marquee.
- **Makers and demos** who want a 7×9 / 6×12 / 12×12 / 18×7 panel showing
  scrolling text or pixel art in a dozen lines of Arduino code.
- **Anyone** who has written `for (led : strip)` coordinate math once and never
  wants to again.

## The 60-second proof

```cpp
#include <Arduino.h>
#include "WuaDisplay_LMX.h"

WuaDisplay display(1);   // ← the ONLY backend-specific line

void setup() {
  display.begin();
  display.setTextColorRGB(0, 180, 255);
  display.startScroll("WUALEDS", 30, WuaDisplay::ScrollMode::Loop);
}

void loop() {
  display.update();      // cooperative, non-blocking
}
```

- Build it with `-D WUA_BOARD_LMX1` → it scrolls across a chain of 7×9 modules.
- Build it with `-D WUA_BOARD_LMX2` → it scrolls across the 6×12 AW20216S.
- Build it with `-D WUA_BOARD_LMX2D` → it scrolls across the 12×12 dual-chip PCB.
- Build it with `-D WUA_BOARD_LMX1P` → it scrolls across the 18×7 panoramic panel.

Same source. One flag. That's the whole point.

➡️ Next: [02 — Getting started](02-getting-started.md).
