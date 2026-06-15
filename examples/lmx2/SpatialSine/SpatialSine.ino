// Example: SpatialSine — animated RGB sine wave across the panel.
//
// Ported from the Wualeds_AW20216S "SpatialSine" example to the high-level
// WuaDisplay API (LMX2 backend). Build a consuming project with
// -D WUA_BOARD_LMX2.
//
// Each pixel's brightness comes from a spatial sine field, phase(x,y) =
// t + x*dx + y*dy, with a different phase offset per color channel. The result
// is a smooth, scrolling plasma. The sine is read from a small quarter-wave LUT
// reconstructed into a full wave by symmetry.
//
// What it shows:
//   - computing per-pixel color from a formula (no stored image).
//   - display.panel().drawPixel() + display.flush() to render a full frame.

#include <Arduino.h>
#include <SPI.h>
#include "WuaDisplay_LMX.h"

// ---- Wiring (placeholder values for ESP32-C3 — adjust to your board) ----
#define PIN_SCK  6
#define PIN_MISO 5
#define PIN_MOSI 7
#define CS_PIN   10

#define WIDTH_LED_MATRIX  6
#define HEIGHT_LED_MATRIX 12

// The concrete backend behind `WuaDisplay` is selected at compile time by the
// active PlatformIO environment (WUA_BOARD_LMX1 / WUA_BOARD_LMX2).
#if defined(WUA_BOARD_LMX2)
  WuaDisplay display(CS_PIN); // LMX2: AW20216S on CS pin 10
#endif

// -------------------- Sine LUT (quarter-wave, 64 samples) --------------------
// Values are sin(theta) scaled to 0..255 for theta in [0..pi/2). We reconstruct
// the full 0..2pi using symmetry and sign, then map to 0..255 brightness.

#if defined(ARDUINO_ARCH_AVR)
  #include <avr/pgmspace.h>
  #define AW_READ_U8(addr) pgm_read_byte(addr)
  const uint8_t kSinQ64[64] PROGMEM = {
#else
  #define AW_READ_U8(addr) (*(const uint8_t*)(addr))
  const uint8_t kSinQ64[64] = {
#endif
    0, 6, 13, 19, 25, 31, 38, 44,
    50, 56, 63, 69, 75, 81, 87, 94,
    100,106,112,118,124,130,136,142,
    148,154,160,166,171,177,183,188,
    194,199,204,210,215,220,225,230,
    235,239,244,248,252,255,255,255,
    255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255
  };

static inline uint8_t sin8_brightness(uint8_t phase)
{
  // phase: 0..255 maps to 0..2pi
  const uint8_t quadrant = (phase >> 6) & 0x03; // 0..3
  const uint8_t idx = phase & 0x3F;             // 0..63

  uint8_t s = AW_READ_U8(&kSinQ64[idx]);
  uint8_t s_mirror = AW_READ_U8(&kSinQ64[63 - idx]);

  // Build signed sine in range [-255..255] as int16
  int16_t signedSin;
  switch (quadrant) {
    case 0: signedSin = (int16_t)s;         break; // 0..+255
    case 1: signedSin = (int16_t)s_mirror;  break; // +255..0
    case 2: signedSin = -(int16_t)s;        break; // 0..-255
    default: signedSin = -(int16_t)s_mirror;break; // -255..0
  }

  // Map [-255..255] -> [0..255]
  int16_t v = (signedSin + 255) >> 1;
  if (v < 0) v = 0;
  if (v > 255) v = 255;
  return (uint8_t)v;
}

static inline uint8_t add8(uint8_t a, uint8_t b) { return (uint8_t)(a + b); }

// -------------------- Spatial animation params --------------------
static const uint8_t kDx = 18;    // phase step per column
static const uint8_t kDy = 11;    // phase step per row
static const uint8_t kSpeed = 1;  // phase increment per frame

// Per-channel phase offsets (spread colors)
static const uint8_t kOffR = 0;
static const uint8_t kOffG = 85;   // ~120 degrees
static const uint8_t kOffB = 170;  // ~240 degrees

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting WuaDisplay LMX2 — SpatialSine...");
  delay(500);
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, CS_PIN);
  delay(50);

  display.begin();
  display.clear();
  display.flush();
}

void loop()
{
  static uint32_t lastMs = 0;
  static uint8_t t = 0;

  const uint32_t now = millis();
  if ((now - lastMs) < 20) return;
  lastMs = now;

  t = (uint8_t)(t + kSpeed);

  // Render spatial wave:
  // phase(x,y) = t + x*dx + y*dy
  // Each channel uses a different phase offset.
  for (uint8_t y = 0; y < HEIGHT_LED_MATRIX; y++) {
    const uint8_t py = (uint8_t)(y * kDy);
    for (uint8_t x = 0; x < WIDTH_LED_MATRIX; x++) {
      const uint8_t phase = (uint8_t)(t + (uint8_t)(x * kDx) + py);

      uint8_t r = sin8_brightness(add8(phase, kOffR));
      uint8_t g = sin8_brightness(add8(phase, kOffG));
      uint8_t b = sin8_brightness(add8(phase, kOffB));

      display.panel().drawPixel(x, y, WuaDisplay::color565(r, g, b));
    }
  }

  display.flush();
}
