// Example: TextScroll — horizontal scrolling marquee text.
//
// Ported from the Wualeds_AW20216S "TextScroll" example to the high-level
// WuaDisplay API (LMX2 backend). Build a consuming project with
// -D WUA_BOARD_LMX2.
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
// Note: the LMX2 panel is only 6 px wide, so at most ~1 character is fully
// visible at a time as the message scrolls through. Use a project that builds
// with -D WUA_BOARD_LMX1 for wider, more readable text.

#include <Arduino.h>
#include <SPI.h>
#include "WuaDisplay_LMX.h"

// ---- Wiring (placeholder values for ESP32-C3 — adjust to your board) ----
#define PIN_SCK  6
#define PIN_MISO 5
#define PIN_MOSI 7
#define CS_PIN   10

// Milliseconds between 1-column scroll steps (higher = slower).
#define SCROLL_MS 120

// The message to scroll.
const char MESSAGE[] = "HELLO WUALABS ";

// The concrete backend behind `WuaDisplay` is selected at compile time by the
// active PlatformIO environment (WUA_BOARD_LMX1 / WUA_BOARD_LMX2).
#if defined(WUA_BOARD_LMX2)
  WuaDisplay display(CS_PIN); // LMX2: AW20216S on CS pin 10
#else
  #error "These examples target the LMX2 backend; build with -D WUA_BOARD_LMX2"
#endif

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting WuaDisplay LMX2 — TextScroll...");
  delay(500);
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, CS_PIN);
  delay(50);

  display.begin();
  display.setTextColorRGB(0, 180, 255); // cyan

  // y = 2 roughly centers the 7px-tall built-in font in the 12-row panel.
  display.startScroll(MESSAGE, SCROLL_MS,
                      WuaDisplay::ScrollMode::Loop,
                      WuaDisplay::ScrollDirection::Left,
                      2);
}

void loop()
{
  display.update(); // non-blocking: clears, draws and flushes each step
}
