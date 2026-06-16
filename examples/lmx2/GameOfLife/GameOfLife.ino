// Example: GameOfLife — Conway's Game of Life on the 6x12 matrix.
//
// Ported from the Wualeds_AW20216S "GameOfLife" example to the high-level
// WuaDisplay API (LMX2 backend). Build a consuming project with
// -D WUA_BOARD_LMX2.
//
// Runs Conway's Game of Life, a classic cellular automaton, on the 6x12 panel.
// Each LED is a cell that is alive (lit) or dead (off). Every generation the
// whole grid is recomputed from four simple rules and redrawn. When the colony
// dies out or freezes, the grid is reseeded with a new random soup. The edges
// wrap around (toroidal grid).
//
// What it shows:
//   - keeping a 2D grid state in RAM and double-buffering each generation.
//   - reading neighbors with wrap-around (modulo) indexing.
//   - display.clear() + display.panel().drawPixel() + display.flush() to render.

#include <Arduino.h>
#include <SPI.h>
#include "WuaDisplay_LMX.h"

// ---- Wiring (placeholder values for ESP32-C3 — adjust to your board) ----
#define PIN_SCK  6
#define PIN_MISO 5
#define PIN_MOSI 7
#define CS_PIN   10

// Row and Column definitions for the 6x12 RGB matrix.
#define WIDTH_LED_MATRIX 6
#define HEIGHT_LED_MATRIX 12

// ── Simulation ────────────────────────────────────────────
#define GEN_MS         180 // Milliseconds between generations.
#define SEED_PERCENT   35  // Approx. % of cells alive when reseeding (0..100).
#define MAX_GEN        200 // Reseed after this many generations (avoids loops).

// Color of a live cell.
#define CELL_R 0
#define CELL_G 255
#define CELL_B 80

// Pin used only to gather analog noise for the random seed (leave unconnected).
// Use any ADC-capable pin on your board (ESP32-C3: GPIO0..4). Adjust as needed.
#define SEED_NOISE_PIN 3

// The concrete backend behind `WuaDisplay` is selected at compile time by the
// active PlatformIO environment (WUA_BOARD_LMX1 / WUA_BOARD_LMX2).
#if defined(WUA_BOARD_LMX2)
  WuaDisplay display(CS_PIN); // LMX2: AW20216S on CS pin 10
#else
  #error "These examples target the LMX2 backend; build with -D WUA_BOARD_LMX2"
#endif

// Two grids: the current generation and the one being computed (double buffer).
uint8_t gridCur[HEIGHT_LED_MATRIX][WIDTH_LED_MATRIX];
uint8_t gridNext[HEIGHT_LED_MATRIX][WIDTH_LED_MATRIX];

//*********************************************************** */
//***********        Simulation helpers                       */
//*********************************************************** */

// Fill the current grid with a fresh random pattern.
static void seedGrid()
{
  for (uint8_t y = 0; y < HEIGHT_LED_MATRIX; y++)
    for (uint8_t x = 0; x < WIDTH_LED_MATRIX; x++)
      gridCur[y][x] = (random(100) < SEED_PERCENT) ? 1 : 0;
}

// Count the live neighbors of cell (x, y), wrapping around the edges.
static uint8_t countNeighbors(uint8_t x, uint8_t y)
{
  uint8_t count = 0;
  for (int8_t dy = -1; dy <= 1; dy++)
  {
    for (int8_t dx = -1; dx <= 1; dx++)
    {
      if (dx == 0 && dy == 0)
        continue; // skip the cell itself

      const uint8_t nx = (uint8_t)((x + dx + WIDTH_LED_MATRIX) % WIDTH_LED_MATRIX);
      const uint8_t ny = (uint8_t)((y + dy + HEIGHT_LED_MATRIX) % HEIGHT_LED_MATRIX);
      count += gridCur[ny][nx];
    }
  }
  return count;
}

// Compute the next generation into gridNext, then swap it into gridCur.
// Returns true if the grid changed (false means it froze and should reseed).
static bool stepGeneration()
{
  bool changed = false;

  for (uint8_t y = 0; y < HEIGHT_LED_MATRIX; y++)
  {
    for (uint8_t x = 0; x < WIDTH_LED_MATRIX; x++)
    {
      const uint8_t n     = countNeighbors(x, y);
      const uint8_t alive = gridCur[y][x];

      // Apply Conway's rules.
      const uint8_t next = alive ? (n == 2 || n == 3) : (n == 3);

      gridNext[y][x] = next;
      if (next != alive)
        changed = true;
    }
  }

  // Swap buffers: copy the freshly computed generation into the current one.
  memcpy(gridCur, gridNext, sizeof(gridCur));
  return changed;
}

// Total number of live cells (used to detect an empty grid).
static uint16_t population()
{
  uint16_t total = 0;
  for (uint8_t y = 0; y < HEIGHT_LED_MATRIX; y++)
    for (uint8_t x = 0; x < WIDTH_LED_MATRIX; x++)
      total += gridCur[y][x];
  return total;
}

// Draw the current grid onto the panel.
static void renderGrid()
{
  display.clear();
  for (uint8_t y = 0; y < HEIGHT_LED_MATRIX; y++)
    for (uint8_t x = 0; x < WIDTH_LED_MATRIX; x++)
      if (gridCur[y][x])
        display.panel().drawPixel(x, y, WuaDisplay::color565(CELL_R, CELL_G, CELL_B));
  display.flush();
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting WuaDisplay LMX2 — GameOfLife...");
  delay(500);
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, CS_PIN);
  delay(50);

  display.begin();

  // Seed the random generator from analog noise, then create the first soup.
  randomSeed((uint32_t)analogRead(SEED_NOISE_PIN) ^ micros());
  seedGrid();
  renderGrid();
}

void loop()
{
  static uint32_t lastMs = 0; // Timestamp of the last generation.
  static uint16_t gen    = 0; // Generation counter since the last reseed.

  // Non-blocking timing: only advance one generation every GEN_MS.
  const uint32_t now = millis();
  if ((now - lastMs) < GEN_MS)
    return;
  lastMs = now;

  // 1. Advance the simulation and draw the new generation.
  const bool changed = stepGeneration();
  gen++;
  renderGrid();

  // 2. Reseed when the colony freezes, dies out, or runs too long.
  if (!changed || population() == 0 || gen >= MAX_GEN)
  {
    Serial.print("Reseeding after ");
    Serial.print(gen);
    Serial.println(" generations.");
    seedGrid();
    renderGrid();
    gen = 0;
  }
}
