/**
 ******************************************************************************
 * @file    ui.c
 * @brief   User interface for the virtual pet (see ui.h).
 *
 * The SPI link to the LCD is slow (a full-screen redraw takes ~0.7 s), so the
 * UI remembers what it last drew and only redraws regions that changed.
 ******************************************************************************
 */
#include "ui.h"
#include "ili9341.h"

#define UI_BG_COLOR         ILI9341_COLOR_BLACK
#define UI_TEXT_COLOR       ILI9341_COLOR_WHITE
#define UI_TEXT_SIZE        2U   /* 5x7 font at x2 -> each char is 12 px wide, 14 px tall */

/* Meter layout. The inner bar is exactly 100 px wide, so 1 px = 1 %. */
#define METER_MAX           100U
#define METER_LABEL_X       8U
#define METER_BAR_X         76U
#define METER_BAR_W         (METER_MAX + 2U)   /* inner width + 1 px outline each side */
#define METER_BAR_H         16U
#define METER_VALUE_X       (METER_BAR_X + METER_BAR_W + 6U)
#define METER_FULLNESS_Y    12U
#define METER_HAPPINESS_Y   36U

/* Value currently shown on each meter; -1 means "not drawn yet" */
static int16_t drawn_fullness  = -1;
static int16_t drawn_happiness = -1;

/* Green when healthy, yellow when getting low, red when critical */
static uint16_t Meter_Color(uint8_t value)
{
  if (value > 50U) { return ILI9341_COLOR_GREEN; }
  if (value > 25U) { return ILI9341_COLOR_YELLOW; }
  return ILI9341_COLOR_RED;
}

/* Formats 0..100 as a right-aligned 3-character string, e.g. "  5", " 42", "100".
 * The leading spaces overwrite any digits left over from a longer number. */
static void Meter_FormatValue(uint8_t value, char out[4])
{
  out[0] = (value >= 100U) ? '1' : ' ';
  out[1] = (value >= 10U) ? (char)('0' + (value / 10U) % 10U) : ' ';
  out[2] = (char)('0' + value % 10U);
  out[3] = '\0';
}

static void Meter_DrawFrame(uint16_t y, const char *label)
{
  ILI9341_DrawString(METER_LABEL_X, (uint16_t)(y + 1U), label, UI_TEXT_COLOR, UI_BG_COLOR, UI_TEXT_SIZE);
  ILI9341_DrawRect(METER_BAR_X, y, METER_BAR_W, METER_BAR_H, UI_TEXT_COLOR);
}

/* Redraws only the inside of one bar plus its number (the outline never changes) */
static void Meter_DrawValue(uint16_t y, uint8_t value)
{
  char text[4];

  if (value > 0U)
  {
    ILI9341_FillRect(METER_BAR_X + 1U, (uint16_t)(y + 1U), value, METER_BAR_H - 2U, Meter_Color(value));
  }
  if (value < METER_MAX)
  {
    ILI9341_FillRect((uint16_t)(METER_BAR_X + 1U + value), (uint16_t)(y + 1U),
                     (uint16_t)(METER_MAX - value), METER_BAR_H - 2U, UI_BG_COLOR);
  }

  Meter_FormatValue(value, text);
  ILI9341_DrawString(METER_VALUE_X, (uint16_t)(y + 1U), text, UI_TEXT_COLOR, UI_BG_COLOR, UI_TEXT_SIZE);
}

void UI_Init(void)
{
  ILI9341_FillScreen(UI_BG_COLOR);

  Meter_DrawFrame(METER_FULLNESS_Y, "FULL");
  Meter_DrawFrame(METER_HAPPINESS_Y, "HAPPY");

  drawn_fullness  = -1;
  drawn_happiness = -1;
}

void UI_DrawMeters(uint8_t fullness, uint8_t happiness)
{
  if (fullness > METER_MAX)  { fullness = METER_MAX; }
  if (happiness > METER_MAX) { happiness = METER_MAX; }

  if (fullness != drawn_fullness)
  {
    Meter_DrawValue(METER_FULLNESS_Y, fullness);
    drawn_fullness = fullness;
  }
  if (happiness != drawn_happiness)
  {
    Meter_DrawValue(METER_HAPPINESS_Y, happiness);
    drawn_happiness = happiness;
  }
}
