// Example: TextScroll — horizontal scrolling marquee text.
//
// Ported from the Wualeds_AW20216S "TextScroll" example to the high-level
// WuaDisplay API. The backend is selected at build time: -D WUA_BOARD_LMX1
// for the 7x9 SK6812 module, -D WUA_BOARD_LMX2 for the 6x12 AW20216S matrix,
// or neither for the default (LMX1) build.
//
// The upstream sketch hand-rolled a 3x5 pixel font and slid a 6-column window
// across a virtual text strip. With this library that whole machinery is
// replaced by the engine's non-blocking scroller, which renders text with the
// built-in Adafruit_GFX font. This example uses only high-level methods (no
// display.panel() needed):
//
//   - display.setTextColorRGB() : set the text color.
//   - display.startScroll()      : start a non-blocking horizontal scroll.
//   - display.update()           : advance the scroll; call it every loop().
//
// Note: both panels are narrow (LMX2 is 6 px wide, one LMX1 module 7 px), so
// only ~1 character is fully visible at a time. Chain several LMX1 modules
// (e.g. WuaDisplay display(7)) for a wider, more readable marquee.

#include <Arduino.h>
#include "WuaDisplay_LMX.h"

// Milliseconds between 1-column scroll steps (higher = slower).
#define SCROLL_MS 120

// The message to scroll.
const char MESSAGE[] = "HELLO WUALABS ";

// The concrete backend behind `WuaDisplay` is selected at compile time by the
// active PlatformIO environment (WUA_BOARD_LMX1 / WUA_BOARD_LMX2).
#if defined(WUA_BOARD_LMX2)
  #include <SPI.h>
  // ---- Wiring (placeholder values for ESP32-C3 — adjust to your board) ----
  #define PIN_SCK  6
  #define PIN_MISO 5
  #define PIN_MOSI 7
  #define CS_PIN   10
  WuaDisplay display(CS_PIN); // LMX2: AW20216S on CS pin 10
#elif defined(WUA_BOARD_LMX1)
  // One LMX1 module is a 7x9 SK6812 RGB matrix. The high-level scroller sizes
  // itself from display.panel(), so no WIDTH/HEIGHT defines are needed here.
  #define LMX1_LED_PIN 5
  WuaDisplay display(1);      // LMX1: N modules chained
#else
  #define WIDTH_LED_MATRIX 7
  #define HEIGHT_LED_MATRIX 9
  WuaDisplay display(7);      // WuaDisplay (7 modules LMX1 chained)
#endif

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting WuaDisplay — TextScroll...");
  delay(500);

#if defined(WUA_BOARD_LMX2)
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, CS_PIN);
#endif

  delay(50);

  display.begin();
  display.setTextColorRGB(0, 180, 255); // cyan

  // y = 2 offsets the 7px-tall built-in font from the top edge (this roughly
  // centers it on the 12-row LMX2 panel; tweak for other panel heights).
  display.startScroll(MESSAGE, SCROLL_MS,
                      WuaDisplay::ScrollMode::Loop,
                      WuaDisplay::ScrollDirection::Left,
                      2);
}

void loop()
{
  display.update(); // non-blocking: clears, draws and flushes each step
}
