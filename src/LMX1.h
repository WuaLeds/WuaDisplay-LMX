#ifndef LMX1_H
#define LMX1_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <FastLED.h>
#include <fl/xmap.h>

// Hardware geometry of a single LMX1 module. Override through build flags
// (-D LMX1_MODULE_WIDTH=..., etc.) if the panel ever changes.
#ifndef LMX1_MODULE_WIDTH
#define LMX1_MODULE_WIDTH 7
#endif
#ifndef LMX1_MODULE_HEIGHT
#define LMX1_MODULE_HEIGHT 18
#endif
#ifndef LMX1_LED_PIN
#define LMX1_LED_PIN 5
#endif

// ============================================================================
//  LMX1 -- Layer 1 pixel panel for the LMX1 backend.
// ----------------------------------------------------------------------------
//  N modules of LMX1_MODULE_WIDTH x LMX1_MODULE_HEIGHT chained horizontally and
//  driven as SK6812 addressable LEDs through FastLED. Exposed as a single
//  (LMX1_MODULE_WIDTH * numModules) x LMX1_MODULE_HEIGHT Adafruit_GFX canvas;
//  drawPixel() maps logical (x, y) to the physical LED index (modules are
//  wired right-to-left).
// ============================================================================
class LMX1 : public Adafruit_GFX
{
public:
    explicit LMX1(uint8_t numModules);
    ~LMX1();

    // Panel contract (see WuaDisplayLMX.h).
    void begin();
    void clear();
    void flush();
    void blur(uint8_t amount);

    // Adafruit_GFX primitive.
    void drawPixel(int16_t x, int16_t y, uint16_t color) override;

private:
    uint16_t mapIndex(int16_t x, int16_t y) const;
    uint16_t ledCount() const { return static_cast<uint16_t>(_panelWidth) * LMX1_MODULE_HEIGHT; }

    uint8_t _numModules;
    int16_t _panelWidth;
    CRGB *_leds;
    fl::XYMap _xyMap;
};

#endif // LMX1_H
