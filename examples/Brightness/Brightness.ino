// Example: Brightness — master brightness control on both backends.
//
// Shows the high-level display.setBrightness(0..255) call, which sets the panel
// master brightness at the hardware level and works the same on either backend:
//   - LMX1: a global FastLED output scale, applied on the next flush().
//   - LMX2: the AW20216S global current register, applied immediately.
//
// What it shows:
//   - display.setBrightness(level) : 0 (off) .. 255 (full).
//   - the same sketch dims/brightens identically on LMX1 and LMX2.
//
// The sketch fills the panel with a solid color and ramps the brightness up and
// down forever, so you see it breathe from black to full and back.
//
// Power note: on LMX2, brightness maps to the AW20216S global current, so full
// brightness (255) drives roughly 4x the current of begin()'s default (0x40) --
// make sure your supply can source it, or lower the top of the ramp. LMX1 is
// bounded by FastLED's setMaxPowerInVoltsAndMilliamps(), so it self-limits.

#include <Arduino.h>
#include "WuaDisplay_LMX.h"

// How fast the brightness ramp moves.
#define STEP        4   // brightness units per step (bigger = faster ramp)
#define STEP_MS     20  // ms between steps

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
#else
  // One LMX1 module is a 7x9 SK6812 RGB matrix.
  #define LMX1_LED_PIN 5
  WuaDisplay display(1);      // LMX1: N modules chained
#endif

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting WuaDisplay — Brightness...");
  delay(500);

#if defined(WUA_BOARD_LMX2)
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, CS_PIN);
#endif

  delay(50);

  display.begin();

  // Draw a solid color once; the loop only changes the master brightness.
  display.panel().fillScreen(WuaDisplay::color565(0, 180, 255)); // cyan
  display.flush();
}

// Ramp the brightness from `start` to `end` (inclusive), flushing each step so
// LMX1 (which scales at show() time) tracks the change too. The last step snaps
// onto `end` so the ramp always reaches the exact endpoint (e.g. full 255 / off
// 0) even when `step` does not divide the range evenly.
static void ramp(int16_t start, int16_t end, int16_t step)
{
  for (int16_t level = start;; level += step)
  {
    if ((step > 0) ? (level > end) : (level < end))
      level = end;
    display.setBrightness(static_cast<uint8_t>(level));
    display.flush();
    delay(STEP_MS);
    if (level == end)
      break;
  }
}

void loop()
{
  ramp(0, 255, STEP);   // fade up to full
  ramp(255, 0, -STEP);  // fade back down to off
}
