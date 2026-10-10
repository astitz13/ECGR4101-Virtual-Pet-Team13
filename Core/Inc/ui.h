/**
 ******************************************************************************
 * @file    ui.h
 * @brief   User interface for the virtual pet: screens, menu and status
 *          meters drawn on the X-NUCLEO-GFX01M2 LCD.
 *
 * The UI only draws. Pet state (fullness, happiness, ...) is owned by the
 * pet logic and passed in; the UI never changes it directly.
 ******************************************************************************
 */
#ifndef UI_H
#define UI_H

#include <stdint.h>

/* Clears the screen and draws the fixed parts of the main pet screen
 * (meter labels and outlines). Call once after ILI9341_Init(). */
void UI_Init(void);

/* Updates the fullness and happiness meters (each 0..100, larger values are
 * clamped to 100). A meter is only redrawn when its value changed since the
 * last call, so this is cheap enough to call on every main-loop pass. */
void UI_DrawMeters(uint8_t fullness, uint8_t happiness);

#endif /* UI_H */
