// Example: Blur — soften and bloom whatever is on the panel.
//
// Demonstrates the blur effect, part of the opt-in effects engine. The backend
// is selected the usual way (-D WUA_BOARD_LMX1 / -D WUA_BOARD_LMX2), but the
// blur API only exists when WUA_ENABLE_EFFECTS is defined. This sketch defines
// it itself (below) so it compiles under any environment; alternatively pass it
// as a build flag (-D WUA_ENABLE_EFFECTS).
//
// What it shows:
//   - draw something crisp with raw GFX primitives (a bright block, then 'W').
//   - display.applyBlur(amount) : neighbour-averaging blur on the live buffer.
//     Calling it repeatedly between flushes makes the shape spread into a soft
//     glow and slowly fade — a classic "bloom" effect.
//
// Backend note: blur needs a readable framebuffer. LMX1 uses FastLED blur2d on
// its LED array; LMX2 keeps a RAM shadow buffer and blurs that in software (the
// AW20216S buffer itself cannot be read back). Either way the shape blooms and
// fades the same.

// Enable the effects engine for this sketch (must precede the library include).
#define WUA_ENABLE_EFFECTS

#include <Arduino.h>
#include "WuaDisplay_LMX.h"

// How hard to blur per step, how many steps, and the timing.
#define BLUR_AMOUNT 64  // 0..255 — higher spreads/fades faster
#define BLUR_STEPS  10  // how many blur passes before redrawing
#define STEP_MS     90  // ms between blur passes
#define HOLD_MS     500 // ms to hold the crisp shape before blurring

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
  Serial.println("Starting WuaDisplay — Blur...");
  delay(500);

#if defined(WUA_BOARD_LMX2)
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, CS_PIN);
#endif

  delay(50);

  display.begin();
}

// Draw a crisp shape, then blur it pass by pass so it blooms and fades.
static void bloom()
{
  display.flush();
  delay(HOLD_MS);
  for (uint8_t i = 0; i < BLUR_STEPS; ++i)
  {
    display.applyBlur(BLUR_AMOUNT);
    display.flush();
    delay(STEP_MS);
  }
  delay(HOLD_MS);
}

void loop()
{
  const int16_t w = display.panel().width();
  const int16_t h = display.panel().height();

  // Scene 1: a bright 3x3 block in the center -> blooms into a soft round glow.
  display.clear();
  display.panel().fillRect(w / 2 - 1, h / 2 - 1, 3, 3,
                           WuaDisplay::color565(0, 200, 255)); // cyan
  bloom();

  // Scene 2: a bright letter 'W' -> shows the same blur softening text.
  display.clear();
  display.setTextColorRGB(255, 120, 0); // orange
  display.printAligned("W", WuaDisplay::HAlign::Center, (h - 7) / 2);
  bloom();
}
