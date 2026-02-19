#ifndef WUADISPLAY_LMX_H
#define WUADISPLAY_LMX_H

#include <Arduino.h>

// Incluir todas (no pasa nada)
#include <WuaDisplay.h>
#include <AW20216S.h>

class WuaDisplay_LMX
{
public:

    enum ModuleType
    {
        MOD_WuaDisplay,
        MOD_LMX1,
        MOD_LMX2
    };

    // Constructor
    WuaDisplay_LMX(ModuleType type,
                   int pinCS_LMX2 = -1,
                   int pin2 = -1,
                   int pin3 = -1,
                   int pin4 = -1);

    void begin();
    void clear();
    void printAligned(const String &text);
    // void update();

private:

    ModuleType _type;

    // Punteros a cada tipo
    WuaDisplay* _modWuaDisplay = nullptr;
    WuaDisplay* _lmx1 = nullptr;
    AW20216S*   _lmx2 = nullptr;

    // Pines
    int _pinCS_LMX2, _pin2, _pin3, _pin4;
};

#endif
