# 05 — API reference

Every public call on `WuaDisplay`, in the order you'd use them. `WuaDisplay` is
the compile-time alias for `WuaDisplayLMX<Panel>`; the snippets below are
backend-agnostic unless noted.

## The lifecycle, step by step

1. **Construct** — forward the backend's setting to the panel.
2. **`begin()`** — initialise hardware (and disable text wrap).
3. **Draw** — text helpers and/or raw GFX via `panel()`.
4. **`flush()`** — push the framebuffer to the hardware in one burst.
5. **`update()`** — call every `loop()` to advance scroll/effects.

```cpp
WuaDisplay display(1);
void setup() { display.begin(); /* draw */ display.flush(); }
void loop()  { display.update(); }
```

---

## Lifecycle

| Call | What it does |
| --- | --- |
| `WuaDisplay display(args...)` | Construct; `args` forwarded to the active panel (module count / CS pin(s) / none). |
| `void begin()` | Initialise the hardware; sets text wrap off. Call once in `setup()`. |
| `void clear()` | Clear the framebuffer (fast/native per backend). Does **not** push — follow with `flush()`. |
| `void flush()` | Push the framebuffer to the panel in one burst. |
| `Panel &panel()` | Escape hatch to the raw `Adafruit_GFX` panel (lines, bitmaps, fonts, per-pixel). |

---

## Color

| Call | What it does |
| --- | --- |
| `static uint16_t color565(r, g, b)` | Pack an 8-bit RGB triplet into a GFX RGB565 color. |
| `void setTextColorRGB(r, g, b)` | Set the panel's text color from RGB. |
| `void setTextColorRGB(target, r, g, b)` | Same, onto any GFX target (e.g. a `Frame`). *(effects build)* |

```cpp
display.setTextColorRGB(0, 180, 255);              // cyan text
uint16_t red = WuaDisplay::color565(255, 0, 0);    // for raw GFX calls
```

---

## Brightness

| Call | What it does |
| --- | --- |
| `void setBrightness(uint8_t level)` | Master brightness, `0` (off) .. `255` (full), at the hardware level. |

- **LMX1 / LMX1p:** a global FastLED output scale — takes effect on the next
  `flush()`.
- **LMX2 / LMX2d:** the AW20216S global-current register — applied immediately.

```cpp
display.setBrightness(64);   // dim to ~25%
```

---

## Aligned text

| Call | What it does |
| --- | --- |
| `void printAligned(text, align = Center, y = 1)` | Draw horizontally-aligned text on the panel (no clear, no flush). |
| `void printAligned(target, text, align, y)` | Same, into a GFX target / `Frame`. *(effects build)* |
| `void printAlignedRefresh(text, align = Center, y = 1)` | Convenience: `clear()` → `printAligned()` → `flush()`. |

`align` is `WuaDisplay::HAlign::{Left, Center, Right}`.

```cpp
display.printAlignedRefresh("Hi");                          // one-liner
display.printAligned("OK", WuaDisplay::HAlign::Right, 0);   // manual: flush yourself
```

---

## Non-blocking scroll

A cooperative marquee: you start it, then call `update()` often; it advances one
pixel step when its interval elapses and never blocks.

| Call | What it does |
| --- | --- |
| `void startScroll(text, speedMs = 30, mode = Once, dir = Left, y = 1)` | Begin scrolling `text`. |
| `void update()` | Advance the scroll (and effects) if due. Call every `loop()`. |
| `void stopScroll()` | Stop and clear the scroll callback. |
| `bool isScrolling()` | `true` while a scroll is active. |
| `void setScrollCallback(cb)` | `void(*)()` fired each time the text fully exits (per loop / once). |

Enums: `ScrollMode::{Loop, Once}`, `ScrollDirection::{Left, Right}`.

```cpp
display.startScroll("WUALEDS DISPLAY", 30,
                    WuaDisplay::ScrollMode::Loop,
                    WuaDisplay::ScrollDirection::Left);
void loop() { display.update(); }
```

---

## Effects engine *(opt-in)*

These exist only when `WUA_ENABLE_EFFECTS` is defined **before** including the
library (or via `-D WUA_ENABLE_EFFECTS`). Without it, they compile to nothing.

### Blur

| Call | What it does |
| --- | --- |
| `void applyBlur(uint8_t amount)` | Spatial neighbour-averaging blur on the live framebuffer (softens / blooms). |

Works on **all** backends: LMX1/LMX1p blur the FastLED array (`blur2d`); LMX2 and
LMX2d blur a RAM shadow buffer in software (on LMX2d the bloom crosses the seam
between the two chips, so it looks like one continuous surface).

### Frames

| Call | What it does |
| --- | --- |
| `using Frame = GFXcanvas16;` | A full off-screen GFX canvas you draw into with normal GFX/text calls. |
| `Frame *createFrame()` | Allocate a `Frame` already sized to the panel (wrap off). **Caller owns it — `delete` when done.** |

Compose a frame with the same calls you'd use on the panel:

```cpp
WuaDisplay::Frame *f = display.createFrame();
f->fillScreen(WuaDisplay::color565(0, 0, 0));
display.setTextColorRGB(*f, 255, 200, 0);
display.printAligned(*f, "B");
f->drawBitmap(x, y, bmp, w, h, color);   // raw GFX works too
```

### Crossfade

| Call | What it does |
| --- | --- |
| `void startCrossfade(from, to, durationMs, stepMs = 30)` | Non-blocking blend `from → to` over `durationMs`. Frames must match the panel size (mismatches are ignored). |
| `bool isCrossfading()` | `true` while a blend is in progress. |
| `void stopCrossfade()` | Abort the current blend. |

The blend interpolates each RGB565 pixel and writes through `drawPixel()`, so it
is identical on every backend.

```cpp
display.startCrossfade(*a, *b, 600);   // 600 ms A → B
void loop() {
  display.update();                    // advances one step per call
  if (!display.isCrossfading()) { /* queue the next transition */ }
}
```

---

## Quick reference: full surface

```cpp
// lifecycle
WuaDisplay display(args...);
void begin();  void clear();  void flush();  Panel &panel();
void update();                         // advances scroll + crossfade

// color
static uint16_t color565(r, g, b);
void setTextColorRGB(r, g, b);
void setTextColorRGB(target, r, g, b);             // effects build

// brightness
void setBrightness(uint8_t level);

// text
void printAligned(text, align = Center, y = 1);
void printAligned(target, text, align, y);          // effects build
void printAlignedRefresh(text, align = Center, y = 1);

// scroll
void startScroll(text, speedMs, mode, dir, y);
void stopScroll();  bool isScrolling();  void setScrollCallback(cb);

// effects (WUA_ENABLE_EFFECTS)
void applyBlur(uint8_t amount);
using Frame = GFXcanvas16;   Frame *createFrame();
void startCrossfade(from, to, durationMs, stepMs = 30);
bool isCrossfading();  void stopCrossfade();
```

➡️ Next: [06 — Examples](06-examples.md).
