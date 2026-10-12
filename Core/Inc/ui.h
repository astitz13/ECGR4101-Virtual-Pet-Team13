/**
 ******************************************************************************
 * @file    ui.h
 * @brief   User interface for the virtual pet: screens, menu and status
 *          meters drawn on the X-NUCLEO-GFX01M2 LCD.
 *
 * The UI only draws and handles navigation. It never changes pet state
 * itself: UI_Update() returns what the user asked for (feed, play, restart)
 * and the main loop passes that on to the pet logic.
 ******************************************************************************
 */
#ifndef UI_H
#define UI_H

#include <stdint.h>

typedef enum
{
  UI_SCREEN_PET,        /* main screen: meters + pet */
  UI_SCREEN_MENU,       /* action list with highlighted selection */
  UI_SCREEN_FEED,       /* feeding: B1 taps feed the pet */
  UI_SCREEN_GAME,       /* play/game action */
  UI_SCREEN_DEAD,       /* pet died of hunger */
  UI_SCREEN_RAN_AWAY    /* pet ran away from unhappiness */
} UiScreen;

typedef enum
{
  UI_REQUEST_NONE,
  UI_REQUEST_FEED,      /* user fed the pet once (one B1 tap) */
  UI_REQUEST_PLAY,      /* user played with the pet once */
  UI_REQUEST_RESTART    /* user held CENTER on the dead/ran-away screen */
} UiRequest;

/* Clears the screen and shows the main pet screen. Call once after ILI9341_Init(). */
void UI_Init(void);

/* Handles menu navigation for one main-loop pass. pressed/held are the
 * INPUT_xxx flags from Input_GetPressed()/Input_GetHeld(); now_ms is
 * HAL_GetTick(). Returns what the user asked the pet logic to do, if anything. */
UiRequest UI_Update(uint32_t now_ms, uint8_t pressed, uint8_t held);

/* Updates the fullness and happiness meters (each 0..100, larger values are
 * clamped to 100). A meter is only redrawn when its value changed since the
 * last call, so this is cheap enough to call on every main-loop pass.
 * Ignored while the dead/ran-away screen is showing. */
void UI_DrawMeters(uint8_t fullness, uint8_t happiness);

/* Switches to UI_SCREEN_DEAD or UI_SCREEN_RAN_AWAY (any other value is
 * ignored). Safe to call every loop pass: does nothing if already showing. */
void UI_ShowGameOver(UiScreen screen);

/* Returns the screen currently shown. */
UiScreen UI_GetScreen(void);

#endif /* UI_H */
