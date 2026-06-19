// LMX1 is the default backend: active whenever the LMX2 board flag is NOT set
// (mirrors the #else branch in WuaDisplay_LMX.h). Guarding the whole translation
// unit means a consumer building the LMX2 backend never compiles this file, so
// it does not drag in FastLED.
#if !defined(WUA_BOARD_LMX2)

#include "LMX1.h"

// Map logical (x, y) -> physical index in the 1D LED array.
// Modules are wired linearly inside each module, but the modules themselves
// are ordered right-to-left along the chain. numModules is derived from the
// canvas width so the mapping stays valid for any module count.
static uint16_t lmx1LayoutXY(uint16_t x, uint16_t y, uint16_t width, uint16_t /*height*/)
{
    const uint16_t numModules   = width / LMX1_MODULE_WIDTH;
    const uint16_t moduleIndex  = x / LMX1_MODULE_WIDTH;
    const uint16_t localX       = x - moduleIndex * LMX1_MODULE_WIDTH;
    const uint16_t realModule   = (numModules - 1u) - moduleIndex; // right-to-left
    const uint16_t moduleOffset = realModule * (LMX1_MODULE_WIDTH * LMX1_MODULE_HEIGHT);
    const uint16_t localIndex   = y * LMX1_MODULE_WIDTH + localX;
    return moduleOffset + localIndex;
}

LMX1::LMX1(uint8_t numModules)
    : Adafruit_GFX(numModules * LMX1_MODULE_WIDTH, LMX1_MODULE_HEIGHT),
      _numModules(numModules),
      _panelWidth(numModules * LMX1_MODULE_WIDTH),
      _leds(new CRGB[static_cast<uint16_t>(numModules) * LMX1_MODULE_WIDTH * LMX1_MODULE_HEIGHT]),
      _xyMap(fl::XYMap::constructWithUserFunction(
          numModules * LMX1_MODULE_WIDTH, LMX1_MODULE_HEIGHT, lmx1LayoutXY, 0))
{
}

LMX1::~LMX1()
{
    delete[] _leds;
}

void LMX1::begin()
{
    FastLED.addLeds<SK6812, LMX1_LED_PIN, GRB>(_leds, ledCount()); // WS2812B / SK6812 / SK6805
    FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);                // 5V, 500mA
    FastLED.clear();
    FastLED.show();
}

uint16_t LMX1::mapIndex(int16_t x, int16_t y) const
{
    return lmx1LayoutXY(static_cast<uint16_t>(x), static_cast<uint16_t>(y),
                        static_cast<uint16_t>(_panelWidth), LMX1_MODULE_HEIGHT);
}

void LMX1::drawPixel(int16_t x, int16_t y, uint16_t color)
{
    if (x < 0 || x >= _panelWidth || y < 0 || y >= LMX1_MODULE_HEIGHT)
        return;

    // Convert 16-bit GFX color (RGB565) to CRGB (RGB888).
    _leds[mapIndex(x, y)] = CRGB(
        ((color >> 11) & 0x1F) << 3, // R
        ((color >> 5) & 0x3F) << 2,  // G
        (color & 0x1F) << 3);        // B
}

void LMX1::clear()
{
    fillScreen(0);
}

void LMX1::flush()
{
    FastLED.show();
}

void LMX1::blur(uint8_t amount)
{
    blur2d(_leds, static_cast<uint8_t>(_panelWidth), LMX1_MODULE_HEIGHT, amount, _xyMap);
}

void LMX1::setBrightness(uint8_t level)
{
    // FastLED scales every LED at show() time, so the change lands on the next flush().
    FastLED.setBrightness(level);
}

#endif // !WUA_BOARD_LMX2
