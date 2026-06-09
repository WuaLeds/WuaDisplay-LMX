#ifndef LMX2_H
#define LMX2_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <SPI.h>
#include <AW20216S.h>

// Matrix geometry of the LMX2 module. Override through build flags if needed.
#ifndef LMX2_WIDTH
#define LMX2_WIDTH 6
#endif
#ifndef LMX2_HEIGHT
#define LMX2_HEIGHT 12
#endif

// ============================================================================
//  LMX2 -- Layer 1 pixel panel for the LMX2 backend.
// ----------------------------------------------------------------------------
//  Adapts the AW20216S 6x12 RGB matrix (SPI) to the Adafruit_GFX canvas.
//  drawPixel() unpacks the GFX 565 color into the chip's native 8-bit-per-
//  channel setPixel(). Note: at 6 px wide, regular text is barely legible --
//  this panel mainly targets pixel art and effects, but it honours the same
//  contract so the high-level layer stays backend-agnostic.
// ============================================================================
class LMX2 : public Adafruit_GFX
{
public:
    explicit LMX2(uint8_t csPin, SPIClass &spiPort = SPI);

    // Panel contract (see WuaDisplayLMX.h).
    void begin();
    void clear();
    void flush();
    void blur(uint8_t amount); // no-op: the AW20216S buffer is write-only

    // Adafruit_GFX primitive.
    void drawPixel(int16_t x, int16_t y, uint16_t color) override;

private:
    AW20216S _aw;
};

#endif // LMX2_H
