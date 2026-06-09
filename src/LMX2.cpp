#include "LMX2.h"

LMX2::LMX2(uint8_t csPin, SPIClass &spiPort)
    : Adafruit_GFX(LMX2_WIDTH, LMX2_HEIGHT),
      _aw(LMX2_HEIGHT, LMX2_WIDTH, csPin, spiPort) // AW20216S(rows, cols, csPin, spi)
{
}

void LMX2::begin()
{
    if (!_aw.begin())
    {
        Serial.println(F("LMX2: AW20216S not detected"));
        return; // let the caller decide; do not hard-lock the firmware
    }
    _aw.setGlobalCurrent(0x40);
    _aw.setScaling(0xFF, 0xFF, 0xFF);
}

void LMX2::drawPixel(int16_t x, int16_t y, uint16_t color)
{
    if (x < 0 || x >= LMX2_WIDTH || y < 0 || y >= LMX2_HEIGHT)
        return;

    // Unpack 16-bit GFX color (RGB565) into 8-bit-per-channel (RGB888).
    const uint8_t r = ((color >> 11) & 0x1F) << 3;
    const uint8_t g = ((color >> 5) & 0x3F) << 2;
    const uint8_t b = (color & 0x1F) << 3;
    _aw.setPixel(static_cast<uint8_t>(x), static_cast<uint8_t>(y), r, g, b);
}

void LMX2::clear()
{
    _aw.clearScreen();
}

void LMX2::flush()
{
    _aw.show();
}

void LMX2::blur(uint8_t /*amount*/)
{
    // Not supported: the AW20216S framebuffer cannot be read back for blending.
}
