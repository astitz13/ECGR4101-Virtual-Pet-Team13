/**
 ******************************************************************************
 * @file    ui.c
 * @brief   User interface for the virtual pet (see ui.h).
 *
 * The SPI link to the LCD is slow (a full-screen redraw takes ~0.7 s), so the
 * UI remembers what it last drew and only redraws regions that changed:
 * meter bars when their value changes, two menu rows when the highlight
 * moves, and only the part of the screen below the meters on screen changes.
 ******************************************************************************
 */
#include "ui.h"
#include "ili9341.h"
#include "input.h"
#include "anim.h"
#include <string.h>

#define UI_BG_COLOR         ILI9341_COLOR_BLACK
#define UI_TEXT_COLOR       ILI9341_COLOR_WHITE
#define UI_HILITE_COLOR     ILI9341_COLOR_CYAN
#define UI_HINT_COLOR       ILI9341_COLOR_GRAY
#define UI_TEXT_SIZE        2U   /* 5x7 font at x2 -> each char is 12 px wide, 14 px tall */
#define UI_CHAR_W(size)     (6U * (size))   /* 5 px glyph + 1 px gap */

/* Meter layout. The inner bar is exactly 100 px wide, so 1 px = 1 %. */
#define METER_MAX           100U
#define METER_LABEL_X       8U
#define METER_BAR_X         76U
#define METER_BAR_W         (METER_MAX + 2U)   /* inner width + 1 px outline each side */
#define METER_BAR_H         16U
#define METER_VALUE_X       (METER_BAR_X + METER_BAR_W + 6U)
#define METER_FULLNESS_Y    12U
#define METER_HAPPINESS_Y   36U

/* Everything from this row down belongs to the current screen */
#define UI_BODY_Y           60U
#define UI_TITLE_Y          72U
#define UI_HINT_Y           296U

/* Pet area (outlined window) and the animation box centered inside it */
#define PET_AREA_X          40U
#define PET_AREA_Y          88U
#define PET_AREA_W          160U
#define PET_AREA_H          160U
#define PET_ANIM_W              96U
#define PET_ANIM_H              96U
#define PET_ANIM_X              (PET_AREA_X + (PET_AREA_W - PET_ANIM_W) / 2U)
#define PET_ANIM_Y              (PET_AREA_Y + (PET_AREA_H - PET_ANIM_H) / 2U)
#define UI_PROMPT_Y         262U   /* action prompt line below the pet area */

#define PERRY_TEAL          0x04B2U   /* RGB565 for roughly (0, 150, 150) */
#define PERRY_HAT_BROWN     0x6204U   /* RGB565 for roughly (100, 65, 33) */

/* No input on the pet screen for this long -> agent mode (hat on) */
#define AGENT_IDLE_MS       10000U

/* Menu rows */
#define MENU_ROW_X          20U
#define MENU_ROW_W          200U
#define MENU_ROW_H          28U
#define MENU_FIRST_Y        104U

/* Hold CENTER this long on the dead/ran-away screen to restart */
#define RESTART_HOLD_MS     2000U
#define RESTART_BAR_X       20U
#define RESTART_BAR_Y       260U
#define RESTART_BAR_W       200U   /* inner width */
#define RESTART_BAR_H       12U

static const char *const menu_items[] = { "FEED", "GAME" };
#define MENU_COUNT          ((uint8_t)(sizeof(menu_items) / sizeof(menu_items[0])))

static UiScreen screen = UI_SCREEN_PET;
static uint8_t  menu_index;

/* Value currently shown on each meter; -1 means "not drawn yet" */
static int16_t drawn_fullness  = -1;
static int16_t drawn_happiness = -1;

/* Hold-to-restart progress */
static uint8_t  hold_active;
static uint32_t hold_start_ms;
static uint16_t drawn_hold_px;

/* Pet animation shown in the pet area; ui_now_ms is the latest HAL_GetTick()
 * passed to UI_Update(), used when a screen change starts an animation */
static AnimPlayer pet_anim;
static uint32_t   ui_now_ms;

/* Agent mode: time of the last button activity, and a voice heard since the
 * last UI_Update() (set by UI_NotifyVoice()) */
static uint32_t last_input_ms;
static uint8_t  voice_pending;

static uint8_t Screen_IsGameOver(UiScreen s)
{
  return (uint8_t)((s == UI_SCREEN_DEAD) || (s == UI_SCREEN_RAN_AWAY));
}

/* Draws text centered horizontally on the screen */
static void UI_DrawCentered(uint16_t y, const char *text, uint16_t color, uint8_t size)
{
  uint16_t width = (uint16_t)(strlen(text) * UI_CHAR_W(size));
  uint16_t x = (width < ILI9341_WIDTH) ? (uint16_t)((ILI9341_WIDTH - width) / 2U) : 0U;

  ILI9341_DrawString(x, y, text, color, UI_BG_COLOR, size);
}

/* ---------------------------------------------------------------- meters */

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

static void Meters_DrawFrames(void)
{
  Meter_DrawFrame(METER_FULLNESS_Y, "FULL");
  Meter_DrawFrame(METER_HAPPINESS_Y, "HAPPY");

  /* Force the values to be drawn on the next UI_DrawMeters() call */
  drawn_fullness  = -1;
  drawn_happiness = -1;
}

/* ------------------------------------------------------------------ menu */

static void Menu_DrawRow(uint8_t index)
{
  uint16_t y = (uint16_t)(MENU_FIRST_Y + index * MENU_ROW_H);
  uint8_t selected = (uint8_t)(index == menu_index);
  uint16_t bg = selected ? UI_HILITE_COLOR : UI_BG_COLOR;
  uint16_t fg = selected ? UI_BG_COLOR : UI_TEXT_COLOR;

  ILI9341_FillRect(MENU_ROW_X, y, MENU_ROW_W, MENU_ROW_H - 4U, bg);
  ILI9341_DrawString(MENU_ROW_X + 8U, (uint16_t)(y + 5U), menu_items[index], fg, bg, UI_TEXT_SIZE);
}

/* Moves the highlight; only the old and new rows are redrawn */
static void Menu_Select(uint8_t index)
{
  uint8_t old = menu_index;

  menu_index = index;
  Menu_DrawRow(old);
  Menu_DrawRow(index);
}

/* ------------------------------------------------------------ animations */

/* Placeholder pet drawn from rectangles until the sprite frames exist.
 * Clears the animation box, then draws the pet shifted dy px (negative = up). */
static void Placeholder_DrawPerry(uint16_t x, uint16_t y, int16_t dy, uint8_t eyes_closed, uint8_t mouth_open)
{
  uint16_t top = (uint16_t)((int16_t)y + 28 + dy);

  ILI9341_FillRect(x, y, PET_ANIM_W, PET_ANIM_H, UI_BG_COLOR);

  ILI9341_FillRect((uint16_t)(x + 16U), top, 64U, 52U, PERRY_TEAL);                 /* body */
  if (eyes_closed)
  {
    ILI9341_FillRect((uint16_t)(x + 26U), (uint16_t)(top + 15U), 14U, 2U, UI_BG_COLOR);
    ILI9341_FillRect((uint16_t)(x + 56U), (uint16_t)(top + 15U), 14U, 2U, UI_BG_COLOR);
  }
  else
  {
    ILI9341_FillRect((uint16_t)(x + 26U), (uint16_t)(top + 8U), 14U, 14U, UI_TEXT_COLOR);
    ILI9341_FillRect((uint16_t)(x + 56U), (uint16_t)(top + 8U), 14U, 14U, UI_TEXT_COLOR);
    ILI9341_FillRect((uint16_t)(x + 31U), (uint16_t)(top + 13U), 5U, 5U, UI_BG_COLOR);  /* pupils */
    ILI9341_FillRect((uint16_t)(x + 61U), (uint16_t)(top + 13U), 5U, 5U, UI_BG_COLOR);
  }
  ILI9341_FillRect((uint16_t)(x + 24U), (uint16_t)(top + 30U), 48U, 10U, ILI9341_COLOR_ORANGE);  /* beak */
  if (mouth_open)
  {
    ILI9341_FillRect((uint16_t)(x + 24U), (uint16_t)(top + 40U), 48U, 4U, UI_BG_COLOR);
    ILI9341_FillRect((uint16_t)(x + 24U), (uint16_t)(top + 44U), 48U, 6U, ILI9341_COLOR_ORANGE);
  }
  ILI9341_FillRect((uint16_t)(x + 24U), (uint16_t)(top + 52U), 14U, 6U, ILI9341_COLOR_ORANGE);  /* feet */
  ILI9341_FillRect((uint16_t)(x + 58U), (uint16_t)(top + 52U), 14U, 6U, ILI9341_COLOR_ORANGE);
}

/* Agent fedora drawn above the placeholder pet. dy is the pet's offset (same
 * as Placeholder_DrawPerry), hat_dy lifts the hat above its resting spot. */
static void Placeholder_DrawHat(uint16_t x, uint16_t y, int16_t dy, int16_t hat_dy)
{
  uint16_t brim = (uint16_t)((int16_t)y + 24 + dy + hat_dy);

  ILI9341_FillRect((uint16_t)(x + 24U), (uint16_t)(brim - 14U), 48U, 14U, PERRY_HAT_BROWN);  /* crown */
  ILI9341_FillRect((uint16_t)(x + 24U), (uint16_t)(brim - 5U), 48U, 3U, UI_BG_COLOR);       /* band */
  ILI9341_FillRect((uint16_t)(x + 10U), brim, 76U, 5U, PERRY_HAT_BROWN);                     /* brim */
}

/* Half-closes the eyes for the agent's serious look */
static void Placeholder_NarrowEyes(uint16_t x, uint16_t y, int16_t dy)
{
  uint16_t top = (uint16_t)((int16_t)y + 28 + dy);

  ILI9341_FillRect((uint16_t)(x + 26U), (uint16_t)(top + 8U), 14U, 7U, PERRY_TEAL);
  ILI9341_FillRect((uint16_t)(x + 56U), (uint16_t)(top + 8U), 14U, 7U, PERRY_TEAL);
}

/* Idle: gentle bob with a blink on frame 2 */
static void Idle_Draw(uint8_t frame, uint16_t x, uint16_t y)
{
  static const int8_t bob[4] = { 0, 2, 4, 2 };
  Placeholder_DrawPerry(x, y, bob[frame], (uint8_t)(frame == 2U), 0U);
}

/* Eating: food drops in (frames 0-2), then chomps (frames 3-5) */
static void Eat_Draw(uint8_t frame, uint16_t x, uint16_t y)
{
  static const uint8_t food_y[3] = { 0U, 8U, 16U };

  Placeholder_DrawPerry(x, y, 0, 0U, (uint8_t)((frame < 3U) || (frame == 4U)));
  if (frame < 3U)
  {
    ILI9341_FillRect((uint16_t)(x + 42U), (uint16_t)(y + food_y[frame]), 12U, 12U, ILI9341_COLOR_YELLOW);
  }
}

/* Playing: two happy jumps */
static void Play_Draw(uint8_t frame, uint16_t x, uint16_t y)
{
  static const int8_t jump[7] = { 0, -8, -14, -8, 0, -6, 0 };
  Placeholder_DrawPerry(x, y, jump[frame], 0U, (uint8_t)(jump[frame] < 0));
}

/* Agent mode starts: the hat drops onto his head, then the eyes narrow */
static void HatOn_Draw(uint8_t frame, uint16_t x, uint16_t y)
{
  static const int8_t hat_dy[5] = { -10, -6, -3, 0, 0 };

  Placeholder_DrawPerry(x, y, 0, 0U, 0U);
  Placeholder_DrawHat(x, y, 0, hat_dy[frame]);
  if (frame == 4U)
  {
    Placeholder_NarrowEyes(x, y, 0);
  }
}

/* Agent idle: hat on, eyes narrowed, slow sway */
static void AgentIdle_Draw(uint8_t frame, uint16_t x, uint16_t y)
{
  int16_t dy = (frame == 0U) ? 0 : 2;

  Placeholder_DrawPerry(x, y, dy, 0U, 0U);
  Placeholder_DrawHat(x, y, dy, 0);
  Placeholder_NarrowEyes(x, y, dy);
}

/* Called back: the hat lifts off and he looks happy */
static void HatOff_Draw(uint8_t frame, uint16_t x, uint16_t y)
{
  static const int8_t hat_dy[4] = { -3, -6, -10, 0 };

  Placeholder_DrawPerry(x, y, 0, 0U, (uint8_t)(frame == 3U));
  if (frame < 3U)
  {
    Placeholder_DrawHat(x, y, 0, hat_dy[frame]);
  }
}

static const Animation anim_idle       = { Idle_Draw,       4U, 300U, 1U };
static const Animation anim_eat        = { Eat_Draw,        6U, 180U, 0U };
static const Animation anim_play       = { Play_Draw,       7U, 120U, 0U };
static const Animation anim_hat_on     = { HatOn_Draw,      5U, 200U, 0U };
static const Animation anim_agent_idle = { AgentIdle_Draw,  2U, 600U, 1U };
static const Animation anim_hat_off    = { HatOff_Draw,     4U, 180U, 0U };

/* --------------------------------------------------------------- screens */

static void Restart_DrawBar(void)
{
  ILI9341_DrawRect(RESTART_BAR_X, RESTART_BAR_Y, RESTART_BAR_W + 2U, RESTART_BAR_H, UI_TEXT_COLOR);
  drawn_hold_px = 0U;
}

/* Redraws only the text below the pet area, so the pet screen can switch in
 * and out of agent mode without touching the animation */
static void PetScreen_DrawPrompts(void)
{
  ILI9341_FillRect(0U, UI_PROMPT_Y, ILI9341_WIDTH, (uint16_t)(ILI9341_HEIGHT - UI_PROMPT_Y), UI_BG_COLOR);

  if (screen == UI_SCREEN_AGENT)
  {
    UI_DrawCentered(UI_PROMPT_Y, "AGENT MODE", ILI9341_COLOR_ORANGE, UI_TEXT_SIZE);
    UI_DrawCentered(UI_HINT_Y, "MAKE A SOUND TO CALL PERRY BACK", UI_HINT_COLOR, 1U);
  }
  else
  {
    UI_DrawCentered(UI_HINT_Y, "CENTER: MENU", UI_HINT_COLOR, UI_TEXT_SIZE);
  }
}

static void Screen_DrawBody(void)
{
  uint8_t i;

  switch (screen)
  {
    case UI_SCREEN_PET:
    case UI_SCREEN_AGENT:
      ILI9341_DrawRect(PET_AREA_X, PET_AREA_Y, PET_AREA_W, PET_AREA_H, UI_HINT_COLOR);
      PetScreen_DrawPrompts();
      break;

    case UI_SCREEN_MENU:
      UI_DrawCentered(UI_TITLE_Y, "MENU", UI_TEXT_COLOR, UI_TEXT_SIZE);
      for (i = 0U; i < MENU_COUNT; i++)
      {
        Menu_DrawRow(i);
      }
      UI_DrawCentered(UI_HINT_Y, "UP/DOWN: MOVE  CENTER: OK  LEFT: BACK", UI_HINT_COLOR, 1U);
      break;

    case UI_SCREEN_FEED:
      UI_DrawCentered(UI_TITLE_Y, "FEEDING", UI_TEXT_COLOR, UI_TEXT_SIZE);
      ILI9341_DrawRect(PET_AREA_X, PET_AREA_Y, PET_AREA_W, PET_AREA_H, UI_HINT_COLOR);
      UI_DrawCentered(UI_PROMPT_Y, "PRESS B1 TO FEED", UI_TEXT_COLOR, UI_TEXT_SIZE);
      UI_DrawCentered(UI_HINT_Y, "LEFT: BACK", UI_HINT_COLOR, UI_TEXT_SIZE);
      break;

    case UI_SCREEN_GAME:
      UI_DrawCentered(UI_TITLE_Y, "GAME", UI_TEXT_COLOR, UI_TEXT_SIZE);
      ILI9341_DrawRect(PET_AREA_X, PET_AREA_Y, PET_AREA_W, PET_AREA_H, UI_HINT_COLOR);
      UI_DrawCentered(UI_PROMPT_Y, "CENTER: PLAY", UI_TEXT_COLOR, UI_TEXT_SIZE);
      UI_DrawCentered(UI_HINT_Y, "LEFT: BACK", UI_HINT_COLOR, UI_TEXT_SIZE);
      break;

    case UI_SCREEN_DEAD:
      UI_DrawCentered(100U, "PERRY", ILI9341_COLOR_RED, 3U);
      UI_DrawCentered(130U, "DIED", ILI9341_COLOR_RED, 3U);
      UI_DrawCentered(200U, "HOLD CENTER", UI_TEXT_COLOR, UI_TEXT_SIZE);
      UI_DrawCentered(224U, "TO RESTART", UI_TEXT_COLOR, UI_TEXT_SIZE);
      Restart_DrawBar();
      break;

    case UI_SCREEN_RAN_AWAY:
      UI_DrawCentered(100U, "PERRY", ILI9341_COLOR_YELLOW, 3U);
      UI_DrawCentered(130U, "RAN AWAY", ILI9341_COLOR_YELLOW, 3U);
      UI_DrawCentered(200U, "HOLD CENTER", UI_TEXT_COLOR, UI_TEXT_SIZE);
      UI_DrawCentered(224U, "TO RESTART", UI_TEXT_COLOR, UI_TEXT_SIZE);
      Restart_DrawBar();
      break;

    default:
      break;
  }
}

/* Switches screens. Normal screens share the meters at the top, so only the
 * body below them is cleared; game-over screens use the whole display. */
static void Screen_Show(UiScreen next, uint8_t full_redraw)
{
  if (Screen_IsGameOver(next) || Screen_IsGameOver(screen))
  {
    full_redraw = 1U;
  }
  screen = next;
  Anim_Stop(&pet_anim);
  last_input_ms = ui_now_ms;   /* agent-mode countdown restarts on every screen change */
  voice_pending = 0U;

  if (full_redraw)
  {
    ILI9341_FillScreen(UI_BG_COLOR);
    if (!Screen_IsGameOver(next))
    {
      Meters_DrawFrames();
    }
  }
  else
  {
    ILI9341_FillRect(0U, UI_BODY_Y, ILI9341_WIDTH, (uint16_t)(ILI9341_HEIGHT - UI_BODY_Y), UI_BG_COLOR);
  }

  Screen_DrawBody();

  /* Screens with a pet area start out with the idle animation */
  if ((next == UI_SCREEN_PET) || (next == UI_SCREEN_FEED) || (next == UI_SCREEN_GAME))
  {
    Anim_Start(&pet_anim, &anim_idle, PET_ANIM_X, PET_ANIM_Y, ui_now_ms);
  }
}

/* Hold CENTER to restart: fills the progress bar while held, resets it on release.
 * Returns 1 once the hold has lasted RESTART_HOLD_MS. */
static uint8_t Restart_UpdateHold(uint32_t now_ms, uint8_t held)
{
  uint32_t elapsed;
  uint16_t px;

  if ((held & INPUT_CENTER) == 0U)
  {
    if (hold_active)
    {
      hold_active = 0U;
      ILI9341_FillRect(RESTART_BAR_X + 1U, RESTART_BAR_Y + 1U, RESTART_BAR_W, RESTART_BAR_H - 2U, UI_BG_COLOR);
      drawn_hold_px = 0U;
    }
    return 0U;
  }

  if (!hold_active)
  {
    hold_active = 1U;
    hold_start_ms = now_ms;
  }

  elapsed = now_ms - hold_start_ms;
  if (elapsed >= RESTART_HOLD_MS)
  {
    hold_active = 0U;
    return 1U;
  }

  /* Only draw the newly filled part of the bar */
  px = (uint16_t)((elapsed * RESTART_BAR_W) / RESTART_HOLD_MS);
  if (px > drawn_hold_px)
  {
    ILI9341_FillRect((uint16_t)(RESTART_BAR_X + 1U + drawn_hold_px), RESTART_BAR_Y + 1U,
                     (uint16_t)(px - drawn_hold_px), RESTART_BAR_H - 2U, UI_TEXT_COLOR);
    drawn_hold_px = px;
  }
  return 0U;
}

/* ------------------------------------------------------------ public API */

void UI_Init(void)
{
  menu_index = 0U;
  hold_active = 0U;
  ui_now_ms = HAL_GetTick();
  screen = UI_SCREEN_PET;
  Screen_Show(UI_SCREEN_PET, 1U);
}

UiRequest UI_Update(uint32_t now_ms, uint8_t pressed, uint8_t held)
{
  UiRequest request = UI_REQUEST_NONE;

  ui_now_ms = now_ms;
  if ((pressed | held) != 0U)
  {
    last_input_ms = now_ms;
  }

  /* Advance the pet animation; when a one-shot ends, return to the matching idle loop */
  if (Anim_Update(&pet_anim, now_ms))
  {
    Anim_Start(&pet_anim, (screen == UI_SCREEN_AGENT) ? &anim_agent_idle : &anim_idle,
               PET_ANIM_X, PET_ANIM_Y, now_ms);
  }

  switch (screen)
  {
    case UI_SCREEN_PET:
      if (pressed & INPUT_CENTER)
      {
        menu_index = 0U;
        Screen_Show(UI_SCREEN_MENU, 0U);
      }
      else if (((now_ms - last_input_ms) >= AGENT_IDLE_MS) && (Anim_Current(&pet_anim) == &anim_idle))
      {
        /* Left alone too long: hat goes on */
        screen = UI_SCREEN_AGENT;
        voice_pending = 0U;
        PetScreen_DrawPrompts();
        Anim_Start(&pet_anim, &anim_hat_on, PET_ANIM_X, PET_ANIM_Y, now_ms);
      }
      break;

    case UI_SCREEN_AGENT:
      /* A voice (reported by UI_NotifyVoice) or any button calls him back.
       * Only the voice counts as an interaction for the pet logic; the
       * button press is used up by waking him. */
      if (voice_pending || (pressed != 0U))
      {
        if (voice_pending)
        {
          request = UI_REQUEST_VOICE;
        }
        voice_pending = 0U;
        screen = UI_SCREEN_PET;
        last_input_ms = now_ms;
        PetScreen_DrawPrompts();
        Anim_Start(&pet_anim, &anim_hat_off, PET_ANIM_X, PET_ANIM_Y, now_ms);
      }
      break;

    case UI_SCREEN_MENU:
      if ((pressed & INPUT_UP) && (menu_index > 0U))
      {
        Menu_Select((uint8_t)(menu_index - 1U));
      }
      else if ((pressed & INPUT_DOWN) && (menu_index < (uint8_t)(MENU_COUNT - 1U)))
      {
        Menu_Select((uint8_t)(menu_index + 1U));
      }
      else if (pressed & INPUT_LEFT)
      {
        Screen_Show(UI_SCREEN_PET, 0U);
      }
      else if (pressed & INPUT_CENTER)
      {
        Screen_Show((menu_index == 0U) ? UI_SCREEN_FEED : UI_SCREEN_GAME, 0U);
      }
      break;

    case UI_SCREEN_FEED:
      if (pressed & INPUT_B1)
      {
        request = UI_REQUEST_FEED;
        Anim_Start(&pet_anim, &anim_eat, PET_ANIM_X, PET_ANIM_Y, now_ms);
      }
      if (pressed & INPUT_LEFT)
      {
        Screen_Show(UI_SCREEN_PET, 0U);
      }
      break;

    case UI_SCREEN_GAME:
      if (pressed & INPUT_CENTER)
      {
        request = UI_REQUEST_PLAY;
        Anim_Start(&pet_anim, &anim_play, PET_ANIM_X, PET_ANIM_Y, now_ms);
      }
      if (pressed & INPUT_LEFT)
      {
        Screen_Show(UI_SCREEN_PET, 0U);
      }
      break;

    case UI_SCREEN_DEAD:
    case UI_SCREEN_RAN_AWAY:
      if (Restart_UpdateHold(now_ms, held))
      {
        request = UI_REQUEST_RESTART;
        Screen_Show(UI_SCREEN_PET, 1U);
      }
      break;

    default:
      break;
  }

  return request;
}

void UI_DrawMeters(uint8_t fullness, uint8_t happiness)
{
  if (Screen_IsGameOver(screen))
  {
    return;
  }

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

void UI_ShowGameOver(UiScreen which)
{
  if (!Screen_IsGameOver(which) || (screen == which))
  {
    return;
  }
  hold_active = 0U;
  Screen_Show(which, 1U);
}

void UI_NotifyVoice(void)
{
  if (screen == UI_SCREEN_AGENT)
  {
    voice_pending = 1U;
  }
}

UiScreen UI_GetScreen(void)
{
  return screen;
}
