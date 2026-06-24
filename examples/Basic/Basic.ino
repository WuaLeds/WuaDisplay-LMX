// Example: Basic — moving pixel + full-screen flash.
//
// Ported from the Wualeds_AW20216S "Basic" example to the high-level
// WuaDisplay API. The backend is selected at build time: -D WUA_BOARD_LMX1
// for the 7x9 SK6812 module, -D WUA_BOARD_LMX2 for the 6x12 AW20216S matrix,
// -D WUA_BOARD_LMX2D for the 12x12 two-chip board, -D WUA_BOARD_LMX1P for the
// 18x7 panoramic panel, or neither for the default (LMX1) build. This sketch is
// size-agnostic (it reads the canvas size from display.panel()), so it runs
// unchanged on any of them.
//
// What it shows:
//   - display.panel()        : raw Adafruit_GFX escape hatch (per-pixel draw).
//   - WuaDisplay::color565()  : pack an RGB triplet into a GFX 565 color.
//   - display.flush()         : push the framebuffer to the panel in one burst.
//
// The original sketch drove the AW20216S directly (setPixel / show); here the
// per-pixel drawing goes through the GFX primitives the active panel exposes
// via display.panel(), while clear()/flush() are high-level WuaDisplay calls.
// The loop is size-agnostic: it reads the canvas size from display.panel().

#include <Arduino.h>
#include "WuaDisplay_LMX.h"

// Master brightness is available through the high-level API on both backends
// (display.setBrightness(0..255) — see the Brightness example). White balance,
// where the hardware supports it, is still configured inside the backend's
// begin().

// The concrete backend behind `WuaDisplay` is selected at compile time by the
// active PlatformIO environment (LMX1 / LMX2 / LMX2D / LMX1P board flags).
#if defined(WUA_BOARD_LMX2D)
  #include <SPI.h>
  // ---- Wiring (placeholder values for ESP32-C3 — adjust to your board) ----
  // One PCB with two AW20216S chips on a shared SPI bus, one CS each -> 12x12.
  #define PIN_SCK   6
  #define PIN_MISO  5
  #define PIN_MOSI  7
  #define CS_LEFT   10  // chip driving the left 6x12 half
  #define CS_RIGHT  1   // chip driving the right 6x12 half
  WuaDisplay display(CS_LEFT, CS_RIGHT); // LMX2d: two chips, one 12x12 canvas
#elif defined(WUA_BOARD_LMX2)
  #include <SPI.h>
  // ---- Wiring (placeholder values for ESP32-C3 — adjust to your board) ----
  #define PIN_SCK  6
  #define PIN_MISO 5
  #define PIN_MOSI 7
  #define CS_PIN   10
  WuaDisplay display(CS_PIN); // LMX2: AW20216S on CS pin 10
#elif defined(WUA_BOARD_LMX1P)
  // LMX1p is a fixed 18x7 "panoramic" panel: one PCB with two LMX1 modules
  // chained as a single SK6812 run on one data pin (LMX1P_LED_PIN, default 5).
  // No module count is needed; this sketch sizes itself from display.panel().
  WuaDisplay display;         // LMX1p: fixed 18x7 panoramic panel
#else
  // One LMX1 module is a 7x9 SK6812 RGB matrix. This sketch sizes itself from
  // display.panel(), so no WIDTH/HEIGHT defines are needed here.
  #define LMX1_LED_PIN 5
  WuaDisplay display(1);      // LMX1: N modules chained
#endif

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting WuaDisplay — Basic...");
  delay(500);

#if defined(WUA_BOARD_LMX2D)
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI); // shared bus; each chip has its own CS
#elif defined(WUA_BOARD_LMX2)
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, CS_PIN);
#endif

  delay(50);

  display.begin();
}

void loop()
{
  const int16_t w = display.panel().width();
  const int16_t h = display.panel().height();

  Serial.println("Demo: pixel traveling...");
  for (int16_t y = 0; y < h; y++)
  {
    for (int16_t x = 0; x < w; x++)
    {
      display.panel().drawPixel(x, y, WuaDisplay::color565(255, 0, 0));
      display.flush();
      delay(50);
      display.panel().drawPixel(x, y, WuaDisplay::color565(0, 0, 0));
      display.flush();
    }
  }

  Serial.println("Demo: blue flash");
  display.panel().fillScreen(WuaDisplay::color565(0, 0, 255));
  display.flush();
  delay(500);

  display.clear();
  display.flush();
  delay(500);
}
