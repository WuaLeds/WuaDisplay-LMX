# WuaDisplay-LMX — Documentation

A unified, high-level display API for Wualeds LED panels. Write your sketch
once against `WuaDisplay`; pick the physical hardware with a single build flag.

This folder is the complete guide to the library. Start with the overview, then
follow the track that matches your experience.

## Table of contents

| Doc | What it covers |
| --- | --- |
| [01 — Why WuaDisplay-LMX](01-why-wuadisplay.md) | What the library is for, the pain it removes, and why you'd use it (the pitch). |
| [02 — Getting started](02-getting-started.md) | Three guided tracks: **Beginner**, **Normal**, and **Advanced**. |
| [03 — Architecture](03-architecture.md) | The two-layer design, static polymorphism, the Panel contract, and how each piece fits. |
| [04 — Backends & hardware](04-backends.md) | The architecture of every compatible component: LMX1, LMX2, LMX2d, LMX1p, and the chips behind them. |
| [05 — API reference](05-api-reference.md) | Every high-level call, step by step, with examples. |
| [06 — Examples](06-examples.md) | Every bundled example sketch, explained step by step. |

For AI agents: a self-contained skill file lives in
[`skill/wuadisplay-lmx.md`](skill/wuadisplay-lmx.md).

## The 30-second version

```cpp
#include <Arduino.h>
#include "WuaDisplay_LMX.h"

WuaDisplay display(1);            // backend chosen by a -D build flag

void setup() {
  display.begin();
  display.setBrightness(180);
  display.startScroll("HELLO WUALEDS");   // non-blocking marquee
}

void loop() {
  display.update();               // advances the scroll / effects
}
```

Build the **same** sketch for any panel by flipping one flag:

| Flag | Panel | Canvas |
| --- | --- | --- |
| `-D WUA_BOARD_LMX1` (default) | N chained 7×9 SK6812 modules (FastLED) | `(7·N) × 9` |
| `-D WUA_BOARD_LMX2` | One 6×12 AW20216S matrix (SPI) | `6 × 12` |
| `-D WUA_BOARD_LMX2D` | One PCB, two AW20216S chips (SPI) | `12 × 12` |
| `-D WUA_BOARD_LMX1P` | "Panoramic" PCB, two LMX1 modules (FastLED) | `18 × 7` |

## License

MIT © Wualeds. See [`../LICENSE`](../LICENSE).
