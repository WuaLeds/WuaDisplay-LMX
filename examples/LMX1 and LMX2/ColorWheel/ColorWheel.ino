// Example: ColorWheel / RainbowFill — cycle the whole panel through the rainbow.
//
// Ported from the Wualeds_AW20216S "ColorWheel" example to the high-level
// WuaDisplay API. The backend is selected at build time: -D WUA_BOARD_LMX1
// for the 7x9 SK6812 module, -D WUA_BOARD_LMX2 for the 6x12 AW20216S matrix,
// or neither for the default (LMX1) build.
//
// The whole matrix is filled with a single, solid color that slowly sweeps
// around the color wheel (red -> green -> blue -> back to red).
//
// What it shows:
//   - generating color on the fly with a compact integer HSV -> RGB.
//   - display.panel().fillScreen() : paint the entire framebuffer with one color.
//   - display.flush()              : push the framebuffer to the panel in one burst.
//
// Note: in the original sketch master brightness (setGlobalCurrent) and white
// balance (setScaling) were set here in setup(); with this library they live
// inside the backend's begin() and are not exposed by the high-level API.

#include <Arduino.h>
#include "WuaDisplay_LMX.h"

// How fast the rainbow advances and how often we redraw a frame.
#define HUE_STEP  1   // Hue increment per frame (1..255). Higher = faster.
#define FRAME_MS  30  // Milliseconds between frames.

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
  // One LMX1 module is a 7x9 SK6812 RGB matrix. fillScreen() covers the whole
  // canvas, so no WIDTH/HEIGHT defines are needed here.
  #define LMX1_LED_PIN 5
  WuaDisplay display(1);      // LMX1: N modules chained
#else
  #define WIDTH_LED_MATRIX 7
  #define HEIGHT_LED_MATRIX 9
  WuaDisplay display(7);      // WuaDisplay (7 modules LMX1 chained)
#endif

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
  Serial.println("Starting WuaDisplay — ColorWheel...");
  delay(500);

#if defined(WUA_BOARD_LMX2)
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, CS_PIN);
#endif

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
