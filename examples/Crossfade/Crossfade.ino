// Example: Crossfade — smooth blend between arbitrary frames.
//
// Demonstrates the effects engine, which is opt-in at build time. The backend
// is still selected the usual way (-D WUA_BOARD_LMX1 / -D WUA_BOARD_LMX2), but
// the crossfade API only exists when WUA_ENABLE_EFFECTS is defined. This sketch
// defines it itself (below) so it compiles under any environment; alternatively
// pass it as a build flag (-D WUA_ENABLE_EFFECTS).
//
// The point of this example is that a Frame can hold ANYTHING you can draw with
// Adafruit_GFX. It cycles forever through five very different frames:
//   1. letter 'W', no background
//   2. letter 'W', on a colored background
//   3. a symbol (heart bitmap), no background
//   4. the same symbol, on a colored background
//   5. a plain background, nothing on top
// ...crossfading from each frame to the next.
//
// What it shows:
//   - display.createFrame()    : allocate a Frame already sized to the panel.
//   - composing a frame with the SAME high-level calls used on the panel
//     (display.printAligned(*frame, ...), display.setTextColorRGB(*frame, ...))
//     and raw GFX too (fillScreen, drawBitmap...).
//   - display.startCrossfade() : non-blocking blend from one frame to the next.
//   - display.isCrossfading()  : true while a blend is in progress.
//   - display.update()         : advances the blend; call it every loop().
//
// The blend runs entirely in the high-level layer and is written through the
// panel's drawPixel(), so the exact same sketch works on LMX1 and LMX2.

// Enable the effects engine for this sketch (must precede the library include).
#define WUA_ENABLE_EFFECTS

#include <Arduino.h>
#include "WuaDisplay_LMX.h"

// Crossfade duration (frame -> next) and the hold time on each frame, in ms.
#define FADE_MS 600
#define HOLD_MS 500

// The concrete backend behind `WuaDisplay` is selected at compile time by the
// active PlatformIO environment (WUA_BOARD_LMX1 / WUA_BOARD_LMX2).
#if defined(WUA_BOARD_LMX2)
  #include <SPI.h>
  // ---- Wiring (placeholder values for ESP32-C3 — adjust to your board) ----
  #define PIN_SCK  18
  #define PIN_MISO 19
  #define PIN_MOSI 23
  #define CS_PIN   5
  WuaDisplay display(CS_PIN); // LMX2: AW20216S on CS pin 10
#else
  // One LMX1 module is a 7x9 SK6812 RGB matrix.
  #define LMX1_LED_PIN 5
  WuaDisplay display(1);      // LMX1: N modules chained
#endif

// ---- A small 5x5 heart bitmap, used as the "symbol" frame. -----------------
// 1 bit per pixel, MSB first; rows are byte-aligned (only the top 5 bits used).
//   . X . X .
//   X X X X X
//   X X X X X
//   . X X X .
//   . . X . .
static const uint8_t HEART_W = 5, HEART_H = 5;
static const uint8_t kHeart[] = {0x50, 0xF8, 0xF8, 0x70, 0x20};

// The cycle of frames to blend through, allocated in setup() with createFrame()
// (which sizes each one to the panel automatically).
static const uint8_t FRAME_COUNT = 5;
WuaDisplay::Frame *frames[FRAME_COUNT] = {nullptr};
uint8_t current = 0; // frame currently shown on screen

// ---- Frame composition helpers --------------------------------------------

// Vertically-centered Y for built-in-font text on this frame. The font's visible
// glyph is 5x7 px (inside a 6x8 cell), so center against 7, not the cell height.
static int16_t centeredTextY(WuaDisplay::Frame &f) { return (f.height() - 7) / 2; }

// Centered heart bitmap, drawn with a raw GFX primitive (drawBitmap).
static void drawCenteredHeart(WuaDisplay::Frame &f, uint16_t color)
{
  f.drawBitmap((f.width() - HEART_W) / 2, (f.height() - HEART_H) / 2,
               kHeart, HEART_W, HEART_H, color);
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting WuaDisplay — Crossfade...");
  delay(500);

#if defined(WUA_BOARD_LMX2)
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, CS_PIN);
#endif

  delay(50);

  display.begin();

  for (uint8_t i = 0; i < FRAME_COUNT; ++i)
    frames[i] = display.createFrame();

  const uint16_t black = WuaDisplay::color565(0, 0, 0);

  // 1) Letter 'W', no background (cyan on black). Note these are the SAME
  //    high-level calls used to draw on the panel, just targeting the frame.
  frames[0]->fillScreen(black);
  display.setTextColorRGB(*frames[0], 0, 180, 255);
  display.printAligned(*frames[0], "W", WuaDisplay::HAlign::Center, centeredTextY(*frames[0]));

  // 2) Letter 'W' on a background (amber 'W' over purple).
  frames[1]->fillScreen(WuaDisplay::color565(120, 0, 160));
  display.setTextColorRGB(*frames[1], 255, 200, 0);
  display.printAligned(*frames[1], "W", WuaDisplay::HAlign::Center, centeredTextY(*frames[1]));

  // 3) Symbol (heart), no background (red on black).
  frames[2]->fillScreen(black);
  drawCenteredHeart(*frames[2], WuaDisplay::color565(255, 0, 40));

  // 4) Symbol on a background (red heart over teal).
  frames[3]->fillScreen(WuaDisplay::color565(0, 90, 90));
  drawCenteredHeart(*frames[3], WuaDisplay::color565(255, 30, 30));

  // 5) Plain background, nothing on top (orange).
  frames[4]->fillScreen(WuaDisplay::color565(255, 90, 0));
}

void loop()
{
  // When idle, hold the current frame briefly, then blend to the next one.
  // The index wraps, so the whole sequence repeats forever.
  if (!display.isCrossfading())
  {
    delay(HOLD_MS);
    const uint8_t next = (current + 1) % FRAME_COUNT;
    display.startCrossfade(*frames[current], *frames[next], FADE_MS);
    current = next;
  }

  display.update(); // non-blocking: advances the blend a step per call
}
