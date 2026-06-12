// Example: ColorWheel / RainbowFill — cycle the whole panel through the rainbow.
//
// Ported from the Wualeds_AW20216S "ColorWheel" example to the high-level
// WuaDisplay API (LMX2 backend). Build a consuming project with
// -D WUA_BOARD_LMX2.
//
// The whole 6x12 matrix is filled with a single, solid color that slowly
// sweeps around the color wheel (red -> green -> blue -> back to red).
//
// What it shows:
//   - generating color on the fly with a compact integer HSV -> RGB.
//   - display.panel().fillScreen() : paint the entire framebuffer with one color.
//   - display.flush()              : push the framebuffer to the chip in one burst.
//
// Note: in the original sketch master brightness (setGlobalCurrent) and white
// balance (setScaling) were set here in setup(); with this library they live
// inside LMX2::begin() and are not exposed by the high-level API.

#include <Arduino.h>
#include <SPI.h>
#include "WuaDisplay_LMX.h"

// ---- Wiring (placeholder values for ESP32-C3 — adjust to your board) ----
#define PIN_SCK  6
#define PIN_MISO 5
#define PIN_MOSI 7
#define CS_PIN   10

// How fast the rainbow advances and how often we redraw a frame.
#define HUE_STEP  1   // Hue increment per frame (1..255). Higher = faster.
#define FRAME_MS  30  // Milliseconds between frames.

WuaDisplay display(CS_PIN);

// Convert a hue (0..255 around the wheel) into an RGB triplet at full
// saturation and full value.
//   hue: 0 = red, 85 = green, 170 = blue, 255 wraps back to red.
static void hueToRgb(uint8_t hue, uint8_t &r, uint8_t &g, uint8_t &b)
{
  const uint8_t sector = hue / 85;       // 0, 1 or 2
  const uint8_t pos    = (hue % 85) * 3; // ramp 0..255 within the sector

  switch (sector)
  {
    case 0: // Red -> Green
      r = 255 - pos; g = pos;       b = 0;
      break;
    case 1: // Green -> Blue
      r = 0;         g = 255 - pos; b = pos;
      break;
    default: // Blue -> Red
      r = pos;       g = 0;         b = 255 - pos;
      break;
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting WuaDisplay LMX2 — ColorWheel...");
  delay(500);
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, CS_PIN);
  delay(50);

  display.begin();
}

void loop()
{
  static uint8_t  hue    = 0; // Current position on the color wheel (0..255).
  static uint32_t lastMs = 0; // Timestamp of the last drawn frame.

  // Non-blocking frame timing: only redraw every FRAME_MS milliseconds.
  const uint32_t now = millis();
  if ((now - lastMs) < FRAME_MS)
    return;
  lastMs = now;

  uint8_t r, g, b;
  hueToRgb(hue, r, g, b);

  display.panel().fillScreen(WuaDisplay::color565(r, g, b));
  display.flush();

  hue += HUE_STEP; // wraps automatically at 255 -> 0
}
