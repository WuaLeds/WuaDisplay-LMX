#ifndef WUADISPLAY_LMX_H
#define WUADISPLAY_LMX_H

// ============================================================================
//  WuaDisplay-LMX -- public entry point
// ----------------------------------------------------------------------------
//  Two-layer display library:
//
//    Layer 2 (high level)  WuaDisplayLMX<Panel>  -- text, scroll, effects
//    Layer 1 (pixel)       LMX1 / LMX2           -- Adafruit_GFX backends
//
//  Pick the backend at COMPILE time with a build flag:
//
//    -D WUA_BOARD_LMX1   (default)   N chained 7x9 SK6812 modules (FastLED)
//    -D WUA_BOARD_LMX2               single 6x12 AW20216S RGB matrix (SPI)
//    -D WUA_BOARD_LMX2D              one PCB with two AW20216S chips -> 12x12 (SPI)
//
//  `WuaDisplay` is the resulting concrete type. Constructor arguments are
//  forwarded to the active panel:
//
//    WuaDisplay display(7);        // LMX1: 7 modules chained -> 49x18 canvas
//    WuaDisplay display(10);       // LMX2: AW20216S on CS pin 10
//    WuaDisplay display(10, 1);    // LMX2d: two chips on CS pins 10 and 1
//
//  Only the active backend's header (and its dependency) is pulled in.
// ============================================================================

#include "WuaDisplayLMX.h"

#if defined(WUA_BOARD_LMX2D)
#include "LMX2D.h"
using WuaDisplay = WuaDisplayLMX<LMX2D>;
#elif defined(WUA_BOARD_LMX2)
#include "LMX2.h"
using WuaDisplay = WuaDisplayLMX<LMX2>;
#else
#include "LMX1.h"
using WuaDisplay = WuaDisplayLMX<LMX1>;
#endif

#endif // WUADISPLAY_LMX_H
