// LMX2d backend: active only when its board flag is set (mirrors WuaDisplay_LMX.h).
// Guarding the whole translation unit means a consumer building another backend
// never compiles this file, so it does not drag in Wualeds_AW20216S.
#if defined(WUA_BOARD_LMX2D)

#include "LMX2D.h"
#include <string.h> // memset

LMX2D::LMX2D(uint8_t csLeft, uint8_t csRight, SPIClass &spiPort)
    : Adafruit_GFX(LMX2D_WIDTH, LMX2D_HEIGHT),
      // AW20216S(rows, cols, csPin, spi). Each chip drives one 6x12 half; they
      // share the same SPI bus and differ only in their chip-select line.
      _left(LMX2_HEIGHT, LMX2_WIDTH, csLeft, spiPort),
      _right(LMX2_HEIGHT, LMX2_WIDTH, csRight, spiPort),
      _fb{} // start blank
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

    // Unpack 16-bit GFX color (RGB565) into 8-bit-per-channel (RGB888), into the
    // RAM shadow buffer for the whole 12x12 canvas. flush() splits it per chip.
    uint8_t *p = &_fb[(y * LMX2D_WIDTH + x) * 3];
    p[0] = ((color >> 11) & 0x1F) << 3; // R
    p[1] = ((color >> 5) & 0x3F) << 2;  // G
    p[2] = (color & 0x1F) << 3;         // B
}

void LMX2D::clear()
{
    memset(_fb, 0, sizeof(_fb)); // pushed to both chips on the next flush()
}

void LMX2D::flush()
{
    // Route every pixel of the shadow buffer to the chip that owns its column,
    // in that chip's local coordinates, then latch both halves.
    for (uint8_t y = 0; y < LMX2D_HEIGHT; ++y)
        for (uint8_t x = 0; x < LMX2D_WIDTH; ++x)
        {
            const uint8_t *p = &_fb[(y * LMX2D_WIDTH + x) * 3];
            if (x < LMX2_WIDTH)
                _left.setPixel(x, y, p[0], p[1], p[2]);
            else
                _right.setPixel(x - LMX2_WIDTH, y, p[0], p[1], p[2]);
        }
    _left.show();
    _right.show();
}

// 8-bit scale and saturating add, matching FastLED's nscale8 / qadd8 semantics
// so the blur looks the same as on LMX1/LMX2.
static inline uint8_t scale8(uint8_t v, uint8_t s) { return (static_cast<uint16_t>(v) * s) >> 8; }
static inline uint8_t qadd8(uint8_t a, uint8_t b)
{
    const uint16_t s = static_cast<uint16_t>(a) + b;
    return s > 255 ? 255 : static_cast<uint8_t>(s);
}

// Blur one channel along a line of `n` pixels spaced `stride` bytes apart, in
// place. Each pixel keeps `keep`/256 of itself and seeps `seep`/256 to each
// neighbour (carried forward, then added back to the previous pixel) -- the same
// running-carry scheme FastLED's blur1d uses.
static void blurLine(uint8_t *base, int n, int stride, uint8_t keep, uint8_t seep)
{
    uint8_t carry = 0;
    for (int i = 0; i < n; ++i)
    {
        uint8_t *p = base + i * stride;
        const uint8_t cur = *p;
        const uint8_t part = scale8(cur, seep);
        if (i)
        {
            uint8_t *prev = base + (i - 1) * stride;
            *prev = qadd8(*prev, part);
        }
        *p = qadd8(scale8(cur, keep), carry);
        carry = part;
    }
}

void LMX2D::blur(uint8_t amount)
{
    // Separable box blur over the whole 12x12 canvas: a horizontal pass over
    // every row, then a vertical pass over every column. Because the shadow
    // buffer spans both chips, the bloom crosses the seam between columns 5 and
    // 6 seamlessly, as on a single continuous surface -- just like LMX2.
    const uint8_t keep = 255 - amount;
    const uint8_t seep = amount >> 1;

    for (int y = 0; y < LMX2D_HEIGHT; ++y)
        for (int c = 0; c < 3; ++c)
            blurLine(&_fb[(y * LMX2D_WIDTH) * 3 + c], LMX2D_WIDTH, 3, keep, seep);

    for (int x = 0; x < LMX2D_WIDTH; ++x)
        for (int c = 0; c < 3; ++c)
            blurLine(&_fb[x * 3 + c], LMX2D_HEIGHT, LMX2D_WIDTH * 3, keep, seep);
}

void LMX2D::setBrightness(uint8_t level)
{
    // The AW20216S global current acts as the master brightness; write it to
    // both chips so the two halves track each other (begin() seeds them at 0x40).
    _left.setGlobalCurrent(level);
    _right.setGlobalCurrent(level);
}

#endif // WUA_BOARD_LMX2D
