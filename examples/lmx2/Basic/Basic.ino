// Example: Basic — moving pixel + full-screen flash.
//
// Ported from the Wualeds_AW20216S "Basic" example to the high-level
// WuaDisplay API (LMX2 backend). Build a consuming project with
// -D WUA_BOARD_LMX2.
//
// What it shows:
//   - display.panel()        : raw Adafruit_GFX escape hatch (per-pixel draw).
//   - WuaDisplay::color565()  : pack an RGB triplet into a GFX 565 color.
//   - display.flush()         : push the framebuffer to the chip in one burst.
//
// The original sketch drove the AW20216S directly (setPixel / show); here the
// per-pixel drawing goes through the GFX primitives the LMX2 panel exposes via
// display.panel(), while clear()/flush() are high-level WuaDisplay calls.

#include <Arduino.h>
#include <SPI.h>
#include "WuaDisplay_LMX.h"

// ---- Wiring (placeholder values for ESP32-C3 — adjust to your board) ----
#define PIN_SCK  6
#define PIN_MISO 5
#define PIN_MOSI 7
#define CS_PIN   10

// Master brightness (global current) and white balance are configured inside
// LMX2::begin(); they are not part of the high-level API.
WuaDisplay display(CS_PIN);

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting WuaDisplay LMX2 — Basic...");
  delay(500);
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, CS_PIN);
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
