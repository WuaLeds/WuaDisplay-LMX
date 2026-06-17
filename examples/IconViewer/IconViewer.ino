// Example: IconViewer — show several color icons, one after another.
//
// Ported from the Wualeds_AW20216S "IconViewer" example to the high-level
// WuaDisplay API. The backend is selected at build time: -D WUA_BOARD_LMX1
// for the 7x9 SK6812 module, -D WUA_BOARD_LMX2 for the 6x12 AW20216S matrix,
// or neither for the default (LMX1) build.
//
// A small gallery of multi-color icons (heart, up arrow, check, cross,
// battery, exclamation) is drawn on the panel. Every few seconds the
// current icon is replaced by the next one, looping forever.
//
// What it shows:
//   - storing bitmaps as a table of "ASCII art" and indexing into them.
//   - display.clear() + display.panel().drawPixel() + display.flush() to render
//     one full frame.
//   - mapping a character to a color through a small palette function.

#include <Arduino.h>
#include "WuaDisplay_LMX.h"

// How long each icon stays on screen before switching to the next one.
#define DISPLAY_MS 1500

// The concrete backend behind `WuaDisplay` is selected at compile time by the
// active PlatformIO environment (WUA_BOARD_LMX1 / WUA_BOARD_LMX2).
#if defined(WUA_BOARD_LMX2)
  #include <SPI.h>
  // ---- Wiring (placeholder values for ESP32-C3 — adjust to your board) ----
  #define PIN_SCK  6
  #define PIN_MISO 5
  #define PIN_MOSI 7
  #define CS_PIN   10
  WuaDisplay display(CS_PIN); // LMX2: AW20216S on CS pin 10
#else
  // One LMX1 module is a 7x9 SK6812 RGB matrix.
  #define WIDTH_LED_MATRIX 7
  #define HEIGHT_LED_MATRIX 9
  #define LMX1_LED_PIN 5
  WuaDisplay display(1);      // LMX1: N modules chained
#endif

//*********************************************************** */
//***********        Icon Bitmaps                             */
//*********************************************************** */
// Each icon is one text row per panel line, one character per pixel:
//   '.' off    R red    G green    B blue
//   Y yellow   C cyan   M magenta  W white   O orange
// Two sets are defined below so each fits its panel: 6 wide x 12 tall for the
// LMX2 matrix, 6 wide x 9 tall for the LMX1 / default builds. Add your own
// icons in the matching block, keeping that block's row/column count.

#if defined(WUA_BOARD_LMX2)
  const char ICONS[][HEIGHT_LED_MATRIX][WIDTH_LED_MATRIX + 1] = {
  { // 0: Heart
    "......",
    ".R..R.",
    "RRRRRR",
    "RRRRRR",
    "RRRRRR",
    ".RRRR.",
    ".RRRR.",
    "..RR..",
    "..RR..",
    "......",
    "......",
    "......",
  },
  { // 1: Arrow up
    "......",
    "..GG..",
    ".GGGG.",
    "GGGGGG",
    "GGGGGG",
    "..GG..",
    "..GG..",
    "..GG..",
    "..GG..",
    "..GG..",
    "..GG..",
    "......",
  },
  { // 2: Check mark
    "......",
    "......",
    ".....G",
    "....GG",
    "...GG.",
    "G..GG.",
    "GG.GG.",
    ".GGGG.",
    ".GGG..",
    "..G...",
    "......",
    "......",
  },
  { // 3: Cross
    "......",
    "R....R",
    "RR..RR",
    ".RRRR.",
    "..RR..",
    "..RR..",
    "..RR..",
    ".RRRR.",
    "RR..RR",
    "R....R",
    "......",
    "......",
  },
  { // 4: Battery
    "..WW..",
    "GGGGGG",
    "G....G",
    "G.GG.G",
    "G.GG.G",
    "G.GG.G",
    "G.GG.G",
    "G.GG.G",
    "G.GG.G",
    "GGGGGG",
    "......",
    "......",
  },
  { // 5: Exclamation
    "..YY..",
    "..YY..",
    "..YY..",
    "..YY..",
    "..YY..",
    "..YY..",
    "..YY..",
    "..YY..",
    "......",
    "..YY..",
    "..YY..",
    "......",
  },
};
#else
  const char ICONS[][HEIGHT_LED_MATRIX][WIDTH_LED_MATRIX + 1] = {
  { // 0: Heart
    "......",
    ".R..R.",
    "RRRRRR",
    "RRRRRR",
    "RRRRRR",
    ".RRRR.",
    ".RRRR.",
    "..RR..",
    "..RR..",
  },
  { // 1: Arrow up
    "......",
    "..GG..",
    ".GGGG.",
    "GGGGGG",
    "GGGGGG",
    "..GG..",
    "..GG..",
    "..GG..",
    "..GG..",
  },
  { // 2: Check mark
    "......",
    ".....G",
    "....GG",
    "...GG.",
    "G..GG.",
    "GG.GG.",
    ".GGGG.",
    ".GGG..",
    "..G...",
  },
  { // 3: Cross
    "R....R",
    "RR..RR",
    ".RRRR.",
    "..RR..",
    "..RR..",
    "..RR..",
    ".RRRR.",
    "RR..RR",
    "R....R",
  },
  { // 4: Battery
    "..WW..",
    "GGGGGG",
    "G....G",
    "G.GG.G",
    "G.GG.G",
    "G.GG.G",
    "G.GG.G",
    "G.GG.G",
    "GGGGGG",
  },
  { // 5: Exclamation
    "..YY..",
    "..YY..",
    "..YY..",
    "..YY..",
    "..YY..",
    "..YY..",
    "......",
    "..YY..",
    "..YY..",
  },
};
#endif

// Number of icons in the gallery (computed automatically).
const uint8_t ICON_COUNT = sizeof(ICONS) / sizeof(ICONS[0]);

// Human-readable names, printed over Serial as each icon appears.
const char *const ICON_NAMES[] = {
  "Heart", "Arrow up", "Check", "Cross", "Battery", "Exclamation"
};

//*********************************************************** */
//***********        Helper functions                         */
//*********************************************************** */

// Translate one bitmap character into an RGB color from the palette.
static void charColor(char c, uint8_t &r, uint8_t &g, uint8_t &b)
{
  switch (c)
  {
    case 'R': r = 255; g = 0;   b = 0;   break; // red
    case 'G': r = 0;   g = 255; b = 0;   break; // green
    case 'B': r = 0;   g = 0;   b = 255; break; // blue
    case 'Y': r = 255; g = 200; b = 0;   break; // yellow
    case 'C': r = 0;   g = 255; b = 255; break; // cyan
    case 'M': r = 255; g = 0;   b = 255; break; // magenta
    case 'W': r = 255; g = 255; b = 255; break; // white
    case 'O': r = 255; g = 90;  b = 0;   break; // orange
    default:  r = 0;   g = 0;   b = 0;   break; // '.' or unknown = off
  }
}

// Render one icon to the panel: clear, plot every pixel, then push the frame.
static void drawIcon(uint8_t index)
{
  display.clear();

  for (uint8_t y = 0; y < HEIGHT_LED_MATRIX; y++)
  {
    for (uint8_t x = 0; x < WIDTH_LED_MATRIX; x++)
    {
      uint8_t r, g, b;
      charColor(ICONS[index][y][x], r, g, b);
      display.panel().drawPixel(x, y, WuaDisplay::color565(r, g, b));
    }
  }

  display.flush();

  Serial.print("Showing icon: ");
  Serial.println(ICON_NAMES[index]);
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting WuaDisplay — IconViewer...");
  delay(500);

#if defined(WUA_BOARD_LMX2)
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, CS_PIN);
#endif

  delay(50);

  display.begin();

  // Draw the first icon right away.
  drawIcon(0);
}

void loop()
{
  static uint8_t  index  = 0; // Currently displayed icon.
  static uint32_t lastMs = 0; // Timestamp of the last icon switch.

  // Non-blocking timing: only advance to the next icon every DISPLAY_MS.
  const uint32_t now = millis();
  if ((now - lastMs) < DISPLAY_MS)
    return;
  lastMs = now;

  // Move to the next icon (wraps back to 0 after the last one) and draw it.
  index = (uint8_t)((index + 1) % ICON_COUNT);
  drawIcon(index);
}
