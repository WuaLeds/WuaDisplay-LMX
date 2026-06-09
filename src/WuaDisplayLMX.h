#ifndef WUADISPLAYLMX_H
#define WUADISPLAYLMX_H

#include <Arduino.h>
#include <utility> // std::forward

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

    // ---- aligned text ------------------------------------------------------
    void printAligned(const String &text, HAlign align = HAlign::Center, int16_t y = 1)
    {
        _panel.setCursor(alignedX(text, align), y);
        _panel.print(text);
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
    void applyBlur(uint8_t amount) { _panel.blur(amount); }

private:
    int16_t alignedX(const String &text, HAlign align)
    {
        int16_t x1, y1;
        uint16_t w, h;
        _panel.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

        int16_t x = 0;
        switch (align)
        {
        case HAlign::Left:   x = 0; break;
        case HAlign::Center: x = (_panel.width() - static_cast<int16_t>(w)) / 2; break;
        case HAlign::Right:  x = _panel.width() - static_cast<int16_t>(w); break;
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
};

#endif // WUADISPLAYLMX_H
