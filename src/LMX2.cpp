// LMX2 backend: active only when its board flag is set (mirrors WuaDisplay_LMX.h).
// Guarding the whole translation unit means a consumer building the LMX1 backend
// never compiles this file, so it does not drag in Wualeds_AW20216S.
#if defined(WUA_BOARD_LMX2)

#include "LMX2.h"
#include <string.h> // memset

LMX2::LMX2(uint8_t csPin, SPIClass &spiPort)
    : Adafruit_GFX(LMX2_WIDTH, LMX2_HEIGHT),
      _aw(LMX2_HEIGHT, LMX2_WIDTH, csPin, spiPort), // AW20216S(rows, cols, csPin, spi)
      _fb{}                                          // start blank
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

    // Unpack 16-bit GFX color (RGB565) into 8-bit-per-channel (RGB888), into the
    // RAM shadow buffer. The chip is updated in bulk by flush().
    uint8_t *p = &_fb[(y * LMX2_WIDTH + x) * 3];
    p[0] = ((color >> 11) & 0x1F) << 3; // R
    p[1] = ((color >> 5) & 0x3F) << 2;  // G
    p[2] = (color & 0x1F) << 3;         // B
}

void LMX2::clear()
{
    memset(_fb, 0, sizeof(_fb)); // pushed to the chip on the next flush()
}

void LMX2::flush()
{
    for (uint8_t y = 0; y < LMX2_HEIGHT; ++y)
        for (uint8_t x = 0; x < LMX2_WIDTH; ++x)
        {
            const uint8_t *p = &_fb[(y * LMX2_WIDTH + x) * 3];
            _aw.setPixel(x, y, p[0], p[1], p[2]);
        }
    _aw.show();
}

// 8-bit scale and saturating add, matching FastLED's nscale8 / qadd8 semantics
// so the blur looks the same as on LMX1.
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

void LMX2::blur(uint8_t amount)
{
    // Separable box blur: a horizontal pass over every row, then a vertical pass
    // over every column. keep + 2*seep ~= 255, so brightness is roughly conserved
    // each pass and only leaks at the edges -- it spreads and slowly fades, just
    // like LMX1's FastLED blur2d.
    const uint8_t keep = 255 - amount;
    const uint8_t seep = amount >> 1;

    for (int y = 0; y < LMX2_HEIGHT; ++y)
        for (int c = 0; c < 3; ++c)
            blurLine(&_fb[(y * LMX2_WIDTH) * 3 + c], LMX2_WIDTH, 3, keep, seep);

    for (int x = 0; x < LMX2_WIDTH; ++x)
        for (int c = 0; c < 3; ++c)
            blurLine(&_fb[x * 3 + c], LMX2_HEIGHT, LMX2_WIDTH * 3, keep, seep);
}

void LMX2::setBrightness(uint8_t level)
{
    // The AW20216S global current acts as the master brightness; it is a register
    // write that applies immediately (begin() seeds it at 0x40).
    _aw.setGlobalCurrent(level);
}

#endif // WUA_BOARD_LMX2
