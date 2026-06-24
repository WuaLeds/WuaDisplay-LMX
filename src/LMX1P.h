#ifndef LMX1P_H
#define LMX1P_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <FastLED.h>
#include <fl/xmap.h> // fl::XYMap, for blur2d
#include "LMX1.h"    // reuse the single-module geometry (LMX1_MODULE_WIDTH/HEIGHT)

// Data pin for the SK6812 chain. FastLED needs the pin as a compile-time
// constant, so it cannot be a runtime constructor argument. Defaults to the
// same pin as a plain LMX1 chain; override through a build flag if needed.
#ifndef LMX1P_LED_PIN
#define LMX1P_LED_PIN LMX1_LED_PIN
#endif

// Geometry of the LMX1p ("panoramic") board: a single PCB carrying two LMX1
// modules, designed to be used in landscape. Each LMX1 module is
// LMX1_MODULE_WIDTH x LMX1_MODULE_HEIGHT (7x9) in its native portrait
// orientation; rotated 90 degrees for landscape use it measures
// LMX1_MODULE_HEIGHT x LMX1_MODULE_WIDTH (9x7). Two of them side by side form a
// single (2 * LMX1_MODULE_HEIGHT) x LMX1_MODULE_WIDTH = 18x7 canvas. Override
// through build flags if a future board changes the layout.
#ifndef LMX1P_WIDTH
#define LMX1P_WIDTH (2 * LMX1_MODULE_HEIGHT)
#endif
#ifndef LMX1P_HEIGHT
#define LMX1P_HEIGHT (LMX1_MODULE_WIDTH)
#endif

// ============================================================================
//  LMX1P -- Layer 1 pixel panel for the LMX1p ("panoramic") backend.
// ----------------------------------------------------------------------------
//  The LMX1p is NOT two separate LMX1 boards wired together: it is one PCB whose
//  two LMX1 modules form a single, continuously chained run of SK6812 LEDs,
//  driven through FastLED on one data pin. Used in landscape it is an 18x7
//  canvas.
//
//  The physical chain runs as a vertical serpentine, column by column from
//  RIGHT to LEFT: the first LED is the top-right pixel, the chain then runs
//  DOWN that rightmost column, hops to the next column on its left and climbs
//  back UP, and so on (boustrophedon). drawPixel() maps logical (x, y) to that
//  physical LED index. It honours the same Panel contract as LMX1/LMX2, so the
//  whole high-level layer (text, scroll, effects) works unchanged.
// ============================================================================
class LMX1P : public Adafruit_GFX
{
public:
    LMX1P();

    // Panel contract (see WuaDisplayLMX.h).
    void begin();
    void clear();
    void flush();
    void blur(uint8_t amount);         // FastLED blur2d over the LED array
    void setBrightness(uint8_t level); // global FastLED output scale (applied on flush)

    // Adafruit_GFX primitive.
    void drawPixel(int16_t x, int16_t y, uint16_t color) override;

private:
    CRGB _leds[LMX1P_WIDTH * LMX1P_HEIGHT];
    fl::XYMap _xyMap; // logical (x, y) -> physical LED index, for blur2d
};

#endif // LMX1P_H
