#ifndef LMX2D_H
#define LMX2D_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <SPI.h>
#include <AW20216S.h>
#include "LMX2.h" // reuse the single-panel geometry (LMX2_WIDTH / LMX2_HEIGHT)

// Geometry of the LMX2d board: two LMX2 sub-panels integrated on a single PCB,
// placed side by side and sharing one SPI bus with an individual chip-select
// each. The two 6x12 halves combine into a single 12x12 canvas
// (2 * LMX2_WIDTH wide, LMX2_HEIGHT tall). Override through build flags if a
// future board changes the layout.
#ifndef LMX2D_WIDTH
#define LMX2D_WIDTH (2 * LMX2_WIDTH)
#endif
#ifndef LMX2D_HEIGHT
#define LMX2D_HEIGHT LMX2_HEIGHT
#endif

// ============================================================================
//  LMX2d -- Layer 1 pixel panel for the LMX2d backend.
// ----------------------------------------------------------------------------
//  The LMX2d is NOT two separate LMX2 boards wired together: it is one PCB with
//  two AW20216S chips, driven over a single SPI bus with one chip-select line
//  per chip. This panel exposes both halves as a single 12x12 Adafruit_GFX
//  canvas: the left chip covers columns [0, LMX2_WIDTH) and the right chip
//  covers columns [LMX2_WIDTH, 2*LMX2_WIDTH). drawPixel() routes each pixel to
//  the right chip and unpacks the GFX 565 color into the chip's native 8-bit-
//  per-channel setPixel(). It honours the same Panel contract as LMX1/LMX2, so
//  the whole high-level layer (text, scroll, effects) works unchanged.
// ============================================================================
class LMX2D : public Adafruit_GFX
{
public:
    LMX2D(uint8_t csLeft, uint8_t csRight, SPIClass &spiPort = SPI);

    // Panel contract (see WuaDisplayLMX.h).
    void begin();
    void clear();
    void flush();
    void blur(uint8_t amount); // no-op: the AW20216S buffer is write-only

    // Adafruit_GFX primitive.
    void drawPixel(int16_t x, int16_t y, uint16_t color) override;

private:
    AW20216S _left;  // chip covering columns [0, LMX2_WIDTH)
    AW20216S _right; // chip covering columns [LMX2_WIDTH, 2*LMX2_WIDTH)
};

#endif // LMX2D_H
