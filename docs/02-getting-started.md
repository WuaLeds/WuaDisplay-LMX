# 02 — Getting started

Three tracks, same library. Pick the one that matches where you are:

- **[Beginner](#beginner)** — never touched this library; just want pixels on a
  panel.
- **[Normal](#normal)** — comfortable with Arduino/PlatformIO; want text,
  scrolling, brightness, and to pick your backend properly.
- **[Advanced](#advanced)** — building product firmware; want the effects
  engine, custom geometry, and to consume the library as a dependency.

WuaDisplay-LMX is an **ESP32** Arduino library built with **PlatformIO**. It is
a *library*, not an app: `src/` ships no `main`, so you compile it by building a
sketch against it.

---

## Beginner

**Goal: light up a single LMX1 module and show a word.**

### Step 1 — Get the toolchain

Install [PlatformIO](https://platformio.org/install) (the VS Code extension is
the easiest). You also need an ESP32 board; the examples target an
`esp32-c3-devkitc-02`.

### Step 2 — Get the library and pick a sketch

Clone the repo. The ready-to-run sketches live in [`examples/`](../examples).
The simplest is [`Basic`](../examples/Basic/Basic.ino): a pixel that travels the
screen plus a full-screen flash.

### Step 3 — Wire one LMX1 module

An LMX1 module is a 7×9 grid of SK6812 LEDs driven over **one data pin**. Connect:

- module **5V** → board 5V
- module **GND** → board GND
- module **DIN** → GPIO **5** (the default `LMX1_LED_PIN`)

> ⚠️ Even one module can pull real current at full white. Power from a proper 5V
> supply, not just USB, if you push bright full-frame colors.

### Step 4 — Build and flash

This repo compiles a sketch by copying it to `src/main.cpp` and building an
environment. The default environment is `lmx1`:

```sh
cp examples/Basic/Basic.ino src/main.cpp
pio run -e lmx1 -t upload
rm src/main.cpp
```

### Step 5 — Make it yours

Open a serial monitor at 115200 baud to see the demo's log. Then try the
one-liner display:

```cpp
WuaDisplay display(1);          // 1 chained module

void setup() {
  display.begin();
  display.printAlignedRefresh("Hi");   // clear + draw centered + flush
}
void loop() {}
```

That's the whole loop: **construct → `begin()` → draw → `flush()`** (or let
`printAlignedRefresh` do the clear+flush for you). You're done with the basics.

---

## Normal

**Goal: choose a backend on purpose, scroll text, and control brightness.**

### Step 1 — Understand the one decision that matters

The backend is a **compile-time** choice (a `-D WUA_BOARD_*` flag); everything
else is the same code. Each backend is a PlatformIO environment in
[`platformio.ini`](../platformio.ini):

| Env | Flag | Hardware | Constructor |
| --- | --- | --- | --- |
| `lmx1` | `WUA_BOARD_LMX1` | N×(7×9) SK6812 chain | `WuaDisplay display(N);` |
| `lmx2` | `WUA_BOARD_LMX2` | 6×12 AW20216S (SPI) | `WuaDisplay display(csPin);` |
| `lmx2d` | `WUA_BOARD_LMX2D` | 12×12 dual AW20216S | `WuaDisplay display(csLeft, csRight);` |
| `lmx1p` | `WUA_BOARD_LMX1P` | 18×7 panoramic | `WuaDisplay display;` |

The constructor arguments are forwarded straight to the panel, so they differ
per backend — but only that one line does.

### Step 2 — Wire your panel

See [04 — Backends & hardware](04-backends.md) for the wiring of each. FastLED
panels (LMX1/LMX1p) need one data pin; AW20216S panels (LMX2/LMX2d) need an SPI
bus and one chip-select per chip, started **before** `display.begin()`:

```cpp
#include <SPI.h>
SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, CS_PIN);
```

### Step 3 — Scroll text (non-blocking)

Narrow panels can't show much static text, so scrolling is the idiom:

```cpp
void setup() {
  display.begin();
  display.setTextColorRGB(0, 180, 255);
  display.startScroll("WUALEDS DISPLAY",
                      30,                              // ms per pixel step
                      WuaDisplay::ScrollMode::Loop);   // repeat forever
}

void loop() {
  display.update();   // call often; it advances one step when due
}
```

`update()` is cooperative — it never blocks, so your `loop()` stays responsive
for buttons, sensors, networking, etc.

### Step 4 — Control brightness

`setBrightness(0..255)` sets the **master** brightness at the hardware level
(FastLED global scale on LMX1/LMX1p, AW20216S global current on LMX2/LMX2d):

```cpp
display.setBrightness(64);   // dim
// ...
display.setBrightness(255);  // full
```

### Step 5 — Draw your own graphics

Anything `Adafruit_GFX` can do is available through `display.panel()`:

```cpp
display.clear();
display.panel().drawLine(0, 0, 10, 8, WuaDisplay::color565(255, 0, 0));
display.panel().drawBitmap(x, y, bitmap, w, h, color);
display.flush();
```

### Step 6 — Compile-check before you flash

Run any example through your env to catch mistakes fast:

```sh
cp examples/TextScroll/TextScroll.ino src/main.cpp && pio run -e lmx2 && rm src/main.cpp
```

---

## Advanced

**Goal: product-grade firmware — effects, custom geometry, library reuse.**

### Step 1 — Turn on the effects engine

Blur and crossfade are **opt-in** at compile time so you don't pay for them
unless you want them. Define `WUA_ENABLE_EFFECTS` **before** including the
library (or pass `-D WUA_ENABLE_EFFECTS`):

```cpp
#define WUA_ENABLE_EFFECTS
#include "WuaDisplay_LMX.h"
```

When it is **not** defined, the crossfade engine and its frame buffers compile
to nothing — no extra members, no allocations.

### Step 2 — Compose off-screen frames and crossfade

A `Frame` is a full off-screen `Adafruit_GFX` canvas (`GFXcanvas16`) sized to
the panel. Draw into it with the **same** calls you use on the panel, then blend
non-blockingly:

```cpp
WuaDisplay::Frame *a = display.createFrame();   // owned by you: delete when done
WuaDisplay::Frame *b = display.createFrame();

a->fillScreen(WuaDisplay::color565(0, 0, 0));
display.printAligned(*a, "A");                  // target overload → draws into the frame

b->fillScreen(WuaDisplay::color565(120, 0, 160));
display.printAligned(*b, "B");

display.startCrossfade(*a, *b, 600);            // 600 ms blend A → B

void loop() {
  display.update();                             // advances the blend a step per call
  if (!display.isCrossfading()) { /* next transition */ }
}
```

The blend runs entirely in Layer 2 and is written through the panel's
`drawPixel()`, so it is **identical across backends**.

### Step 3 — Blur the live buffer

```cpp
display.applyBlur(64);   // neighbour averaging: softens / blooms what's drawn
display.flush();
```

On LMX1/LMX1p this blurs the FastLED LED array; on LMX2/LMX2d it blurs a RAM
shadow buffer in software (the AW20216S framebuffer can't be read back). The
bloom looks the same on each.

### Step 4 — Override geometry when the board changes

Panel dimensions are `#define`s you can override with build flags, e.g.:

```ini
build_flags =
    -D WUA_BOARD_LMX1
    -D LMX1_MODULE_HEIGHT=9
    -D LMX1_LED_PIN=18
```

See [04 — Backends & hardware](04-backends.md) for every overridable macro per
backend.

### Step 5 — Consume it as a dependency

In a downstream PlatformIO project, add the library to `lib_deps` and select a
backend. Because each backend's `.cpp` is guarded by its board flag, a consumer
compiles — and pulls the dependency of — **only** the active backend, even
without this repo's `build_src_filter`:

```ini
[env:my_product]
build_flags = -D WUA_BOARD_LMX1P
lib_deps =
    WuaDisplay_LMX
    fastled/FastLED          ; only the active backend's dep is needed
    adafruit/Adafruit GFX Library
```

### Step 6 — Add your own backend (optional)

A backend is just an `Adafruit_GFX` subclass that satisfies the **Panel
contract** (`begin / clear / flush / blur / setBrightness` + `drawPixel`). Add
your class, wire it into the `#if/#elif` in
[`WuaDisplay_LMX.h`](../src/WuaDisplay_LMX.h), and the entire Layer 2 engine
works on it for free. See [03 — Architecture](03-architecture.md).

➡️ Next: [03 — Architecture](03-architecture.md) · [05 — API reference](05-api-reference.md).
