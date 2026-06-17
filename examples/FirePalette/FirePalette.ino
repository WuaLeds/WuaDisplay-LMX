// Example: FirePalette — palette-mapped fire effect that rises up.
//
// Ported from the Wualeds_AW20216S "FirePalette" example to the high-level
// WuaDisplay API. The backend is selected at build time: -D WUA_BOARD_LMX1
// for the 7x9 SK6812 module, -D WUA_BOARD_LMX2 for the 6x12 AW20216S matrix,
// or neither for the default (LMX1) build.
//
// Simulates a flickering flame that rises from the bottom of the panel.
// A "heat" value is kept for every pixel: the bottom row is constantly reheated
// with random embers, and each frame the heat spreads upward while cooling, so
// the flame fades from white-hot at the base to dark red at the top.
//
// What it shows:
//   - holding a per-pixel scalar field (heat) in RAM and evolving it each frame.
//   - a fire algorithm: reheat the base, then average-and-cool moving upward.
//   - mapping a scalar to RGB through a palette function (heatToColor()).
//
// Note: this keeps its own software heat buffer, so the smoothing is done in RAM
// rather than on the panel. That makes the effect identical on every backend and
// works even on LMX2, where applyBlur() is a no-op (the AW20216S framebuffer
// cannot be read back).

#include <Arduino.h>
#include "WuaDisplay_LMX.h"

// ── Fire tuning ───────────────────────────────────────────
#define FRAME_MS    50  // Milliseconds between frames.
#define COOLING     45  // Max heat lost per row as the flame rises (higher = shorter flames).
#define EMBER_MIN   180 // Lowest heat injected at the base each frame.
#define EMBER_MAX   255 // Highest heat injected at the base each frame.

// Pin used only to gather analog noise for the random seed (leave unconnected).
// Use any ADC-capable pin on your board (ESP32-C3: GPIO0..4). Adjust as needed.
#define SEED_NOISE_PIN 3

// The concrete backend behind `WuaDisplay` is selected at compile time by the
// active PlatformIO environment (WUA_BOARD_LMX1 / WUA_BOARD_LMX2).
#if defined(WUA_BOARD_LMX2)
  #include <SPI.h>
  // ---- Wiring (placeholder values for ESP32-C3 — adjust to your board) ----
  #define PIN_SCK  6
  #define PIN_MISO 5
  #define PIN_MOSI 7
  #define CS_PIN   10
  // One LMX2 module is a 6x12 SK6812 RGB matrix.
  #define WIDTH_LED_MATRIX 6
  #define HEIGHT_LED_MATRIX 12
  WuaDisplay display(CS_PIN); // LMX2: AW20216S on CS pin 10
#else
  // One LMX1 module is a 7x9 SK6812 RGB matrix.
  #define WIDTH_LED_MATRIX 7
  #define HEIGHT_LED_MATRIX 9
  #define LMX1_LED_PIN 5
  WuaDisplay display(1);      // LMX1: N modules chained
#endif

// Per-pixel heat field. y = 0 is the top row, y = HEIGHT-1 is the hot base.
uint8_t heat[HEIGHT_LED_MATRIX][WIDTH_LED_MATRIX];

//*********************************************************** */
//***********        Fire palette                             */
//*********************************************************** */
// Map a heat value (0..255) to a fire color: black -> red -> orange/yellow ->
// white. Swap this function for a different ramp to get ice or plasma instead.

static void heatToColor(uint8_t h, uint8_t &r, uint8_t &g, uint8_t &b)
{
  if (h < 85)
  {
    // Black -> red.
    r = (uint8_t)(h * 3);
    g = 0;
    b = 0;
  }
  else if (h < 170)
  {
    // Red -> yellow (ramp up green).
    r = 255;
    g = (uint8_t)((h - 85) * 3);
    b = 0;
  }
  else
  {
    // Yellow -> white (ramp up blue).
    r = 255;
    g = 255;
    b = (uint8_t)((h - 170) * 3);
  }
}

//*********************************************************** */
//***********        Fire simulation                          */
//*********************************************************** */

// Advance the flame by one frame: reheat the base row, then spread heat upward
// while cooling, and finally render the heat field through the palette.
static void stepFire()
{
  // 1. Reheat the bottom row with random embers (the fuel source).
  const uint8_t base = HEIGHT_LED_MATRIX - 1;
  for (uint8_t x = 0; x < WIDTH_LED_MATRIX; x++)
    heat[base][x] = (uint8_t)random(EMBER_MIN, EMBER_MAX + 1);

  // 2. Propagate heat upward (rows above the base read the rows below them).
  //    Each cell becomes the average of the cells below it, minus some random
  //    cooling. Columns wrap horizontally so flames can lean side to side.
  for (int y = 0; y < (int)base; y++)
  {
    for (uint8_t x = 0; x < WIDTH_LED_MATRIX; x++)
    {
      const uint8_t xl = (uint8_t)((x + WIDTH_LED_MATRIX - 1) % WIDTH_LED_MATRIX);
      const uint8_t xr = (uint8_t)((x + 1) % WIDTH_LED_MATRIX);

      const uint8_t below  = heat[y + 1][x];
      const uint8_t belowL = heat[y + 1][xl];
      const uint8_t belowR = heat[y + 1][xr];
      const uint8_t below2 = (y + 2 < HEIGHT_LED_MATRIX) ? heat[y + 2][x] : below;

      const uint16_t avg = ((uint16_t)below + belowL + belowR + below2) / 4;
      const int16_t cooled = (int16_t)avg - (int16_t)random(0, COOLING);

      heat[y][x] = (cooled < 0) ? 0 : (uint8_t)cooled;
    }
  }

  // 3. Render the heat field through the fire palette.
  for (uint8_t y = 0; y < HEIGHT_LED_MATRIX; y++)
  {
    for (uint8_t x = 0; x < WIDTH_LED_MATRIX; x++)
    {
      uint8_t r, g, b;
      heatToColor(heat[y][x], r, g, b);
      display.panel().drawPixel(x, y, WuaDisplay::color565(r, g, b));
    }
  }
  display.flush();
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting WuaDisplay — FirePalette...");
  delay(500);

#if defined(WUA_BOARD_LMX2)
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, CS_PIN);
#endif

  delay(50);

  display.begin();

  // Seed the random generator and start with a cold (black) field.
  randomSeed((uint32_t)analogRead(SEED_NOISE_PIN) ^ micros());
  memset(heat, 0, sizeof(heat));

  display.clear();
  display.flush();
}

void loop()
{
  static uint32_t lastMs = 0; // Timestamp of the last frame.

  // Non-blocking timing: only advance the fire every FRAME_MS.
  const uint32_t now = millis();
  if ((now - lastMs) < FRAME_MS)
    return;
  lastMs = now;

  stepFire();
}
