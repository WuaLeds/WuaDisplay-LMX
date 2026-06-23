#ifndef WUADISPLAYLMX_H
#define WUADISPLAYLMX_H

#include <Arduino.h>
#include <utility>        // std::forward
#include <Adafruit_GFX.h> // Adafruit_GFX (drawing targets) + GFXcanvas16 (frames)

// Effects (blur + crossfade) are opt-in at COMPILE time, like the board flags.
// Define WUA_ENABLE_EFFECTS to pull in the crossfade engine and its frame
// buffers; when it is not defined that code compiles to nothing (no extra
// members, no Frame allocations).

// ============================================================================
//  WuaDisplayLMX<Panel> -- High-level display engine (Layer 2)
// ----------------------------------------------------------------------------
//  Generic text / scroll / effects layer, written once on top of any pixel
//  panel. Static polymorphism: the backend is chosen at COMPILE time through
//  the `Panel` template argument, so there is no virtual dispatch nor heap on
//  the hot path. The number of physical modules (or any backend-specific
//  setting) is a RUNTIME constructor argument forwarded to the panel.
//
//  A conforming `Panel` must:
//    * derive from Adafruit_GFX -- provides drawPixel(), print(), setCursor(),
//      getTextBounds(), setTextColor(), setTextWrap(), width(), height()
//    * void begin()        initialise the hardware
//    * void clear()        clear the framebuffer (native/fast when possible)
//    * void flush()        push the framebuffer to the hardware
//    * void blur(uint8_t)  buffer-level blur (no-op when unsupported)
//
//  The contract is enforced by use (a missing member is a compile error), so
//  there is no abstract base class and no runtime cost.
// ============================================================================
template <class Panel>
class WuaDisplayLMX
{
public:
    enum class HAlign { Left, Center, Right };
    enum class ScrollMode { Loop, Once };
    enum class ScrollDirection { Left, Right };
    using ScrollCallback = void (*)();

    // Forward construction arguments straight to the panel
    // (e.g. module count for LMX1, or CS pin for LMX2).
    template <class... Args>
    explicit WuaDisplayLMX(Args &&...args)
        : _panel(std::forward<Args>(args)...)
    {
    }

    // ---- lifecycle ---------------------------------------------------------
    void begin()
    {
        _panel.begin();
        _panel.setTextWrap(false);
    }
    void clear() { _panel.clear(); }
    void flush() { _panel.flush(); }

    // Escape hatch for raw GFX drawing (lines, bitmaps, pixels...).
    Panel &panel() { return _panel; }

    // ---- color -------------------------------------------------------------
    static uint16_t color565(uint8_t r, uint8_t g, uint8_t b)
    {
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }
    void setTextColorRGB(uint8_t r, uint8_t g, uint8_t b)
    {
        _panel.setTextColor(color565(r, g, b));
    }
    // Same call, but onto any GFX target -- e.g. a Frame you are composing:
    //   display.setTextColorRGB(*frame, r, g, b);
    void setTextColorRGB(Adafruit_GFX &target, uint8_t r, uint8_t g, uint8_t b)
    {
        target.setTextColor(color565(r, g, b));
    }

    // ---- aligned text ------------------------------------------------------
    // Horizontally aligned text. The default targets the live panel; pass a GFX
    // target as the first argument to draw onto a Frame instead, using the SAME
    // function -- so static text and effect frames are composed identically:
    //   display.printAligned("HELLO");          // straight onto the panel
    //   display.printAligned(*frame, "HELLO");  // into a frame, for a crossfade
    void printAligned(const String &text, HAlign align = HAlign::Center, int16_t y = 1)
    {
        printAligned(_panel, text, align, y);
    }
    void printAligned(Adafruit_GFX &target, const String &text,
                      HAlign align = HAlign::Center, int16_t y = 1)
    {
        target.setCursor(alignedX(target, text, align), y);
        target.print(text);
    }
    void printAlignedRefresh(const String &text, HAlign align = HAlign::Center, int16_t y = 1)
    {
        _panel.clear();
        printAligned(text, align, y);
        _panel.flush();
    }

    // ---- non-blocking scroll ----------------------------------------------
    void startScroll(const String &text,
                     uint16_t speedMs = 30,
                     ScrollMode mode = ScrollMode::Once,
                     ScrollDirection dir = ScrollDirection::Left,
                     int16_t y = 1)
    {
        _scrollText = text;
        _scrollSpeedMs = speedMs;
        _scrollMode = mode;
        _scrollDir = dir;
        _scrollY = y;
        _isScrolling = true;

        int16_t x1, y1;
        uint16_t w, h;
        _panel.getTextBounds(_scrollText, 0, 0, &x1, &y1, &w, &h);
        _scrollTextWidth = w;

        if (_scrollDir == ScrollDirection::Left)
        {
            _scrollX = _panel.width();                              // start off the right edge
            _scrollMinX = -static_cast<int16_t>(_scrollTextWidth);  // done when fully off the left
        }
        else
        {
            _scrollX = -static_cast<int16_t>(_scrollTextWidth);     // start off the left edge
            _scrollMaxX = _panel.width();                           // done when fully off the right
        }
        _lastScrollTime = millis();
    }
    void setScrollCallback(ScrollCallback cb) { _scrollCallback = cb; }
    void stopScroll()
    {
        _isScrolling = false;
        _scrollCallback = nullptr;
    }
    bool isScrolling() const { return _isScrolling; }

    // Call frequently from loop().
    void update()
    {
#ifdef WUA_ENABLE_EFFECTS
        if (_isCrossfading)
        {
            stepCrossfade();
            return;
        }
#endif
        if (!_isScrolling)
            return;

        uint32_t now = millis();
        if (now - _lastScrollTime < _scrollSpeedMs)
            return;
        _lastScrollTime = now;

        _panel.clear();
        _panel.setCursor(_scrollX, _scrollY);
        _panel.print(_scrollText);
        _panel.flush();

        if (_scrollDir == ScrollDirection::Left)
        {
            if (--_scrollX < _scrollMinX)
                onScrollEnd(_panel.width());
        }
        else
        {
            if (++_scrollX > _scrollMaxX)
                onScrollEnd(-static_cast<int16_t>(_scrollTextWidth));
        }
    }

    // ---- effects -----------------------------------------------------------
#ifdef WUA_ENABLE_EFFECTS
    // ---- blur --------------------------------------------------------------
    // Spatial blur (neighbour averaging) on the live framebuffer. Softens and
    // blooms whatever is currently drawn. Implemented on both backends: LMX1 via
    // FastLED blur2d, LMX2 via a software pass over its RAM shadow buffer.
    void applyBlur(uint8_t amount) { _panel.blur(amount); }

    // ---- frames ------------------------------------------------------------
    // A Frame is a full Adafruit_GFX off-screen canvas, so you compose it with
    // the exact same calls you use on the panel: raw GFX primitives (fillScreen,
    // drawBitmap, drawLine...) AND the engine's text helpers via their target
    // overloads (display.printAligned(*frame, ...), display.setTextColorRGB
    // (*frame, ...)). The library owns the blend engine; the content is yours.
    // GFXcanvas16 manages its own heap buffer and is not safe to copy, so always
    // pass and store frames by reference/pointer.
    using Frame = GFXcanvas16;

    // Allocate a Frame already sized to the panel canvas, so it always matches
    // and startCrossfade() never rejects it for a size mismatch. Text wrap is
    // disabled to match the panel. The caller owns the returned object and must
    // `delete` it (typically a one-off in setup()).
    Frame *createFrame()
    {
        Frame *f = new Frame(_panel.width(), _panel.height());
        f->setTextWrap(false);
        return f;
    }

    // ---- crossfade ---------------------------------------------------------
    // Non-blocking blend from one frame to another (Frame A -> ... -> Frame B),
    // advanced from update() just like the scroll. The blend happens entirely
    // in this layer and is written out through the panel's drawPixel(), so it is
    // backend-agnostic (works on LMX1 and LMX2 without any change to the panel).
    //
    //   WuaDisplay::Frame *a = display.createFrame();
    //   WuaDisplay::Frame *b = display.createFrame();
    //   // ...draw into *a and *b with any GFX primitive...
    //   display.startCrossfade(*a, *b, 600);   // 600 ms A -> B
    //   void loop() { display.update(); }
    void startCrossfade(Frame &from, Frame &to,
                        uint16_t durationMs, uint16_t stepMs = 30)
    {
        // Frames must match the panel canvas; ignore mismatches rather than
        // reading out of bounds.
        if (from.width() != _panel.width() || from.height() != _panel.height() ||
            to.width() != _panel.width() || to.height() != _panel.height())
            return;

        _cfFrom = &from;
        _cfTo = &to;
        _cfDurationMs = durationMs == 0 ? 1 : durationMs; // avoid divide-by-zero
        _cfStepMs = stepMs;
        _cfStartMs = millis();
        _cfLastStep = _cfStartMs - stepMs; // render the first step immediately
        _isCrossfading = true;
    }
    bool isCrossfading() const { return _isCrossfading; }
    void stopCrossfade() { _isCrossfading = false; }
#endif // WUA_ENABLE_EFFECTS

private:
    static int16_t alignedX(Adafruit_GFX &g, const String &text, HAlign align)
    {
        int16_t x1, y1;
        uint16_t w, h;
        g.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

        int16_t x = 0;
        switch (align)
        {
        case HAlign::Left:   x = 0; break;
        case HAlign::Center: x = (g.width() - static_cast<int16_t>(w)) / 2; break;
        case HAlign::Right:  x = g.width() - static_cast<int16_t>(w); break;
        }
        return x < 0 ? 0 : x;
    }

    void onScrollEnd(int16_t restartX)
    {
        if (_scrollCallback)
            _scrollCallback();
        if (_scrollMode == ScrollMode::Loop)
            _scrollX = restartX;
        else
            _isScrolling = false;
    }

#ifdef WUA_ENABLE_EFFECTS
    // Linear interpolation between two RGB565 colors, channel by channel.
    // t is the 0..255 blend weight (0 -> a, 255 -> b).
    static uint16_t blend565(uint16_t a, uint16_t b, uint8_t t)
    {
        const uint8_t ar = (a >> 11) & 0x1F, ag = (a >> 5) & 0x3F, ab = a & 0x1F;
        const uint8_t br = (b >> 11) & 0x1F, bg = (b >> 5) & 0x3F, bb = b & 0x1F;
        const uint8_t r = ar + (((int16_t)br - ar) * t) / 255;
        const uint8_t g = ag + (((int16_t)bg - ag) * t) / 255;
        const uint8_t bl = ab + (((int16_t)bb - ab) * t) / 255;
        return (uint16_t(r) << 11) | (uint16_t(g) << 5) | bl;
    }

    // Render one crossfade frame onto the live panel, throttled by _cfStepMs.
    void stepCrossfade()
    {
        const uint32_t now = millis();
        if (now - _cfLastStep < _cfStepMs)
            return;
        _cfLastStep = now;

        const uint32_t elapsed = now - _cfStartMs;
        const uint8_t t = elapsed >= _cfDurationMs
                              ? 255
                              : static_cast<uint8_t>((elapsed * 255u) / _cfDurationMs);

        const uint16_t *from = _cfFrom->getBuffer();
        const uint16_t *to = _cfTo->getBuffer();
        const int16_t w = _panel.width();
        const int16_t h = _panel.height();
        for (int16_t y = 0; y < h; ++y)
            for (int16_t x = 0; x < w; ++x)
            {
                const uint16_t i = static_cast<uint16_t>(y) * w + x;
                _panel.drawPixel(x, y, blend565(from[i], to[i], t));
            }
        _panel.flush();

        if (t == 255)
            _isCrossfading = false;
    }
#endif // WUA_ENABLE_EFFECTS

    Panel _panel;

    // Scroll state
    String _scrollText;
    uint16_t _scrollSpeedMs = 50;
    bool _isScrolling = false;
    ScrollMode _scrollMode = ScrollMode::Once;
    ScrollDirection _scrollDir = ScrollDirection::Left;
    int16_t _scrollX = 0;
    int16_t _scrollY = 1;
    int16_t _scrollMinX = 0;
    int16_t _scrollMaxX = 0;
    uint16_t _scrollTextWidth = 0;
    uint32_t _lastScrollTime = 0;
    ScrollCallback _scrollCallback = nullptr;

#ifdef WUA_ENABLE_EFFECTS
    // Crossfade state
    GFXcanvas16 *_cfFrom = nullptr;
    GFXcanvas16 *_cfTo = nullptr;
    uint32_t _cfStartMs = 0;
    uint32_t _cfLastStep = 0;
    uint16_t _cfDurationMs = 0;
    uint16_t _cfStepMs = 30;
    bool _isCrossfading = false;
#endif
};

#endif // WUADISPLAYLMX_H
