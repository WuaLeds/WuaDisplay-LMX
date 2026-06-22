// LMX1p backend: active only when its board flag is set (mirrors WuaDisplay_LMX.h).
// Like LMX1 it is built on FastLED; guarding the whole translation unit keeps a
// consumer building a non-FastLED backend from compiling this file.
#if defined(WUA_BOARD_LMX1P)

#include "LMX1P.h"

// Map logical (x, y) -> physical index in the 1D LED chain.
// The chain is a vertical serpentine running column by column from RIGHT to
// LEFT. Column 0 is the rightmost one; even columns are wired top-to-bottom and
// odd columns bottom-to-top, so consecutive columns join tip-to-tip.
static uint16_t lmx1pLayoutXY(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    const uint16_t column = (width - 1u) - x;                      // count columns from the right
    const uint16_t offset = (column & 1u) ? (height - 1u - y) : y; // climb up on odd columns
    return column * height + offset;
}

LMX1P::LMX1P()
    : Adafruit_GFX(LMX1P_WIDTH, LMX1P_HEIGHT)
{
}

void LMX1P::begin()
{
    FastLED.addLeds<SK6812, LMX1P_LED_PIN, GRB>(_leds, LMX1P_WIDTH * LMX1P_HEIGHT); // WS2812B / SK6812 / SK6805
    FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);                                 // 5V, 500mA
    FastLED.clear();
    FastLED.show();
}

void LMX1P::drawPixel(int16_t x, int16_t y, uint16_t color)
{
    if (x < 0 || x >= LMX1P_WIDTH || y < 0 || y >= LMX1P_HEIGHT)
        return;

    // Convert 16-bit GFX color (RGB565) to CRGB (RGB888).
    _leds[lmx1pLayoutXY(static_cast<uint16_t>(x), static_cast<uint16_t>(y),
                        LMX1P_WIDTH, LMX1P_HEIGHT)] = CRGB(
        ((color >> 11) & 0x1F) << 3, // R
        ((color >> 5) & 0x3F) << 2,  // G
        (color & 0x1F) << 3);        // B
}

void LMX1P::clear()
{
    fillScreen(0);
}

void LMX1P::flush()
{
    FastLED.show();
}

void LMX1P::blur(uint8_t /*amount*/)
{
    // Effects are deferred until the LMX1p backend lands. Unlike LMX2/LMX2D the
    // FastLED framebuffer can be read back, so this can later reuse LMX1's
    // blur2d() + fl::XYMap path built from lmx1pLayoutXY.
}

#endif // WUA_BOARD_LMX1P
