/**
 ******************************************************************************
 * @file    input.h
 * @brief   Debounced button input for the virtual pet UI: the X-NUCLEO-GFX01M2
 *          joystick (5 directions) and the Nucleo's blue USER button (B1).
 *
 * The pins are already configured as inputs by MX_GPIO_Init() in main.c, so
 * this module needs no init function. Call Input_Update() on every
 * main-loop pass; it never blocks.
 ******************************************************************************
 */
#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

/* Bit flags used by Input_GetPressed() and Input_GetHeld() */
#define INPUT_UP      (1U << 0)
#define INPUT_DOWN    (1U << 1)
#define INPUT_LEFT    (1U << 2)
#define INPUT_RIGHT   (1U << 3)
#define INPUT_CENTER  (1U << 4)   /* joystick pushed straight down */
#define INPUT_B1      (1U << 5)   /* blue USER button on the Nucleo board */

/* Samples all buttons and updates the debounced state. now_ms is HAL_GetTick(). */
void Input_Update(uint32_t now_ms);

/* Returns the buttons that were pressed since the last call, then clears them.
 * Each physical press is reported exactly once, however long it is held. */
uint8_t Input_GetPressed(void);

/* Returns the buttons currently held down (debounced), e.g. for "hold to restart". */
uint8_t Input_GetHeld(void);

#endif /* INPUT_H */
