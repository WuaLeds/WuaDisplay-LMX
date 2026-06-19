// LMX2d backend: active only when its board flag is set (mirrors WuaDisplay_LMX.h).
// Guarding the whole translation unit means a consumer building another backend
// never compiles this file, so it does not drag in Wualeds_AW20216S.
#if defined(WUA_BOARD_LMX2D)

#include "LMX2D.h"

LMX2D::LMX2D(uint8_t csLeft, uint8_t csRight, SPIClass &spiPort)
    : Adafruit_GFX(LMX2D_WIDTH, LMX2D_HEIGHT),
      // AW20216S(rows, cols, csPin, spi). Each chip drives one 6x12 half; they
      // share the same SPI bus and differ only in their chip-select line.
      _left(LMX2_HEIGHT, LMX2_WIDTH, csLeft, spiPort),
      _right(LMX2_HEIGHT, LMX2_WIDTH, csRight, spiPort)
{
}

void LMX2D::begin()
{
    // Initialise both chips independently; report each one that does not answer
    // but keep going, so a single bad half does not hard-lock the firmware.
    if (!_left.begin())
        Serial.println(F("LMX2D: left AW20216S not detected"));
    else
    {
        _left.setGlobalCurrent(0x40);
        _left.setScaling(0xFF, 0xFF, 0xFF);
    }

    if (!_right.begin())
        Serial.println(F("LMX2D: right AW20216S not detected"));
    else
    {
        _right.setGlobalCurrent(0x40);
        _right.setScaling(0xFF, 0xFF, 0xFF);
    }
}

void LMX2D::drawPixel(int16_t x, int16_t y, uint16_t color)
{
    if (x < 0 || x >= LMX2D_WIDTH || y < 0 || y >= LMX2D_HEIGHT)
        return;

    // Unpack 16-bit GFX color (RGB565) into 8-bit-per-channel (RGB888).
    const uint8_t r = ((color >> 11) & 0x1F) << 3;
    const uint8_t g = ((color >> 5) & 0x3F) << 2;
    const uint8_t b = (color & 0x1F) << 3;

    // Route to the chip that owns this column, in its own local coordinates.
    if (x < LMX2_WIDTH)
        _left.setPixel(static_cast<uint8_t>(x), static_cast<uint8_t>(y), r, g, b);
    else
        _right.setPixel(static_cast<uint8_t>(x - LMX2_WIDTH), static_cast<uint8_t>(y), r, g, b);
}

void LMX2D::clear()
{
    _left.clearScreen();
    _right.clearScreen();
}

void LMX2D::flush()
{
    _left.show();
    _right.show();
}

void LMX2D::blur(uint8_t /*amount*/)
{
    // Not supported: the AW20216S framebuffer cannot be read back for blending.
}

#endif // WUA_BOARD_LMX2D
