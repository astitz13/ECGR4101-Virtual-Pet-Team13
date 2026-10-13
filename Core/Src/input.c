/**
 ******************************************************************************
 * @file    input.c
 * @brief   Debounced button input (see input.h).
 *
 * Mechanical buttons "bounce": for a few milliseconds after a press or
 * release the contact flickers between open and closed. A reading is only
 * accepted once all buttons have read the same for INPUT_DEBOUNCE_MS, so one
 * press produces one event instead of several.
 ******************************************************************************
 */
#include "input.h"
#include "main.h"
#include "gfx01m2_conf.h"

#define INPUT_DEBOUNCE_MS   20U
#define INPUT_COUNT         6U

typedef struct
{
  GPIO_TypeDef *port;
  uint16_t      pin;
  GPIO_PinState pressed_level;   /* pin level that means "pressed" */
  uint8_t       mask;            /* INPUT_xxx flag for this button */
} InputPin;

/* Joystick directions are active low (internal pull-ups, see gfx01m2_conf.h).
 * B1 on the Nucleo-144 reads high when pressed (external pull-down). */
static const InputPin input_pins[INPUT_COUNT] =
{
  { JOY_UP_GPIO_Port,     JOY_UP_Pin,     GPIO_PIN_RESET, INPUT_UP     },
  { JOY_DOWN_GPIO_Port,   JOY_DOWN_Pin,   GPIO_PIN_RESET, INPUT_DOWN   },
  { JOY_LEFT_GPIO_Port,   JOY_LEFT_Pin,   GPIO_PIN_RESET, INPUT_LEFT   },
  { JOY_RIGHT_GPIO_Port,  JOY_RIGHT_Pin,  GPIO_PIN_RESET, INPUT_RIGHT  },
  { JOY_CENTER_GPIO_Port, JOY_CENTER_Pin, GPIO_PIN_RESET, INPUT_CENTER },
  { B1_GPIO_Port,         B1_Pin,         GPIO_PIN_SET,   INPUT_B1     },
};

static uint8_t  last_raw;          /* raw reading from the previous call */
static uint32_t raw_changed_ms;    /* when the raw reading last changed */
static uint8_t  held;              /* debounced "currently pressed" buttons */
static uint8_t  pressed_events;    /* new presses not yet read by the caller */

static uint8_t Input_ReadRaw(void)
{
  uint8_t raw = 0U;
  uint8_t i;

  for (i = 0U; i < INPUT_COUNT; i++)
  {
    if (HAL_GPIO_ReadPin(input_pins[i].port, input_pins[i].pin) == input_pins[i].pressed_level)
    {
      raw |= input_pins[i].mask;
    }
  }
  return raw;
}

void Input_Update(uint32_t now_ms)
{
  uint8_t raw = Input_ReadRaw();

  if (raw != last_raw)
  {
    /* Still changing (or bouncing): restart the stability timer */
    last_raw = raw;
    raw_changed_ms = now_ms;
  }
  else if (((now_ms - raw_changed_ms) >= INPUT_DEBOUNCE_MS) && (raw != held))
  {
    /* Stable long enough: accept it. Buttons that just went down become events. */
    pressed_events |= (uint8_t)(raw & (uint8_t)~held);
    held = raw;
  }
}

uint8_t Input_GetPressed(void)
{
  uint8_t events = pressed_events;

  pressed_events = 0U;
  return events;
}

uint8_t Input_GetHeld(void)
{
  return held;
}
