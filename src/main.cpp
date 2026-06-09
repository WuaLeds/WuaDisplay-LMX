#include <Arduino.h>
#include "WuaDisplay_LMX.h"

// The concrete backend behind `WuaDisplay` is selected at compile time by the
// active PlatformIO environment (WUA_BOARD_LMX1 / WUA_BOARD_LMX2).
#if defined(WUA_BOARD_LMX2)
WuaDisplay display(10); // LMX2: AW20216S on CS pin 10
#else
WuaDisplay display(7);  // LMX1: 7 modules chained
#endif

void setup()
{
    Serial.begin(115200);
    display.begin();
    display.setTextColorRGB(80, 50, 80);

    display.clear();
    display.printAligned("Hi");
    display.flush();

    display.startScroll("Hello WuaLeds!", 30,
                        WuaDisplay::ScrollMode::Loop,
                        WuaDisplay::ScrollDirection::Left);
}

void loop()
{
    display.update();
}
