#include "WuaDisplay_LMX.h"

WuaDisplay_LMX::WuaDisplay_LMX(ModuleType type,
                               int pinCS_LMX2,
                               int pin2,
                               int pin3,
                               int pin4)
{
    _type = type;
    _pinCS_LMX2 = pinCS_LMX2;
    _pin2 = pin2;
    _pin3 = pin3;
    _pin4 = pin4;

    switch (_type)
    {
        case MOD_WuaDisplay:{
            uint8_t numModules = 7; // ejemplo para 7 módulos
            _modWuaDisplay = new WuaDisplay(numModules);
            break;
        }
            

        case MOD_LMX1:{
            uint8_t lmx1 = 1;
            _lmx1 = new WuaDisplay(lmx1);
            break;
        }
            

        case MOD_LMX2:{
            uint8_t WIDTH_LED_MATRIX = 6;
            uint8_t HEIGHT_LED_MATRIX = 12;

            _lmx2 = new AW20216S(HEIGHT_LED_MATRIX, WIDTH_LED_MATRIX, _pinCS_LMX2, SPI); // ejemplo I2C
            break;
        }
            
    }
}

void WuaDisplay_LMX::begin()
{
    switch (_type)
    {
        case MOD_WuaDisplay:
            _modWuaDisplay->begin();
            break;

        case MOD_LMX1:
            _lmx1->begin();
            break;

        case MOD_LMX2:{
            if (!_lmx2->begin())
            {
                Serial.println("Error: AW20216S chip not detected.");
                while (1); // Stop execution if it fails
            }

            // 2. Configure global current (Master brightness)
            // Range 0-255. 0x80 is approximately 50% power.
            _lmx2->setGlobalCurrent(0x40); // Adjust according to desired consumption

            // 3. Set Scaling (White Balance) to maximum
            // This ensures that when you set PWM to maximum, the LED will shine at full brightness.
            _lmx2->setScaling(0xFF, 0xFF, 0xFF);
            Serial.println("AW20216S begin");
            _lmx2->begin();
            Serial.println("AW20216S OK");
            break;
        }
           
    }
}

void WuaDisplay_LMX::clear()
{
    switch (_type)
    {
        case MOD_WuaDisplay:
            _modWuaDisplay->clear();
            break;

        case MOD_LMX1:
            _lmx1->clear();
            break;

        case MOD_LMX2:
            _lmx2->clearScreen();
            break;
    }
}

void WuaDisplay_LMX::printAligned(const String &text)
{
    switch (_type)
    {
        case MOD_WuaDisplay:{
            _modWuaDisplay->clear();
            _modWuaDisplay->setTextColorRGB(8, 0, 0);
            _modWuaDisplay->printAligned(text);
            _modWuaDisplay->refresh();
            break;
        }
            

        case MOD_LMX1:{
            _lmx1->clear();
            _lmx1->setTextColorRGB(8, 0, 0);
            _lmx1->printAligned(text);
            _lmx1->refresh();
            break;
        }

        case MOD_LMX2:{
            Serial.println("AW20216S print");
            _lmx2->fillScreen(0, 0, 255);
            _lmx2->show();
            break;
        }
            
    }
}

// void WuaDisplay_LMX::update()
// {
//     switch (_type)
//     {
//         case MOD_WuaDisplay:
//             _modWuaDisplay->update();
//             break;

//         case MOD_LMX1:
//             _lmx1->update();
//             break;

//         case MOD_LMX2:
//             _lmx2->update();
//             break;
//     }
// }

