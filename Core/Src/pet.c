/**
 * @file pet.c
 * @brief Hardware-independent virtual-pet state machine.
 */
#include "pet.h"

typedef struct
{
  /* Current meter values exposed through the getter functions. */
  uint8_t fullness;
  uint8_t happiness;
  bool demo_mode;
  bool paused;

  /* Current visual/behavioral state and the time it began. */
  PetState state;
  uint32_t state_start_ms;

  /* Timestamps retain fractional decay time between main-loop passes. */
  uint32_t last_fullness_decay_ms;
  uint32_t last_happiness_decay_ms;
} Pet;

/* This module manages one pet, so its state is private to this source file. */
static Pet pet;

static uint32_t Pet_GetFullnessDecayIntervalMs(void)
{
  return pet.demo_mode ? PET_DEMO_FULLNESS_DECAY_INTERVAL_MS :
                         PET_FULLNESS_DECAY_INTERVAL_MS;
}

static uint32_t Pet_GetHappinessDecayIntervalMs(void)
{
  return pet.demo_mode ? PET_DEMO_HAPPINESS_DECAY_INTERVAL_MS :
                         PET_HAPPINESS_DECAY_INTERVAL_MS;
}

/* Enter a permanent terminal state when either meter reaches its limit. */
static void Pet_CheckTerminalState(uint32_t now_ms)
{
  if (pet.fullness <= PET_VALUE_MIN)
  {
    pet.state = PET_STATE_DEAD;
    pet.state_start_ms = now_ms;
  }
  else if (pet.happiness <= PET_VALUE_MIN)
  {
    pet.state = PET_STATE_RAN_AWAY;
    pet.state_start_ms = now_ms;
  }
}

static void Pet_IncreaseValue(uint8_t *value, uint32_t amount)
{
  /* Saturate at 100 instead of allowing an 8-bit value to wrap around. */
  uint32_t increased = (uint32_t)*value + amount;
  *value = (increased > PET_VALUE_MAX) ? PET_VALUE_MAX : (uint8_t)increased;
}

static void Pet_DecreaseValue(uint8_t *value, uint32_t amount)
{
  /* Saturate at 0 instead of allowing an unsigned value to underflow. */
  *value = (amount >= *value) ? PET_VALUE_MIN : (uint8_t)(*value - amount);
}

void Pet_Init(uint32_t now_ms)
{
  /* A new pet begins full, happy, and not performing an action. */
  pet.fullness = PET_VALUE_MAX;
  pet.happiness = PET_VALUE_MAX;
  pet.demo_mode = false;
  pet.state = PET_STATE_IDLE;
  pet.state_start_ms = now_ms;
  pet.last_fullness_decay_ms = now_ms;
  pet.last_happiness_decay_ms = now_ms;
  pet.paused = false;
}

void Pet_Update(uint32_t now_ms)
{
  uint32_t elapsed_fullness_ms;
  uint32_t elapsed_happiness_ms;

  if ((pet.paused) || (pet.state == PET_STATE_DEAD) || (pet.state == PET_STATE_RAN_AWAY))
  {
    /* Terminal states do not decay and must be reset explicitly. */
    return;
  }

  /*
   * Compute how many full intervals elapsed. Keeping the remainder by
   * advancing the timestamp prevents loop speed from changing decay rates.
   */
  uint32_t fullness_interval_ms = Pet_GetFullnessDecayIntervalMs();
  uint32_t happiness_interval_ms = Pet_GetHappinessDecayIntervalMs();

  elapsed_fullness_ms = now_ms - pet.last_fullness_decay_ms;
  if (elapsed_fullness_ms >= fullness_interval_ms)
  {
    uint32_t steps = elapsed_fullness_ms / fullness_interval_ms;
    Pet_DecreaseValue(&pet.fullness, steps);
    pet.last_fullness_decay_ms += steps * fullness_interval_ms;
  }

  elapsed_happiness_ms = now_ms - pet.last_happiness_decay_ms;
  if (elapsed_happiness_ms >= happiness_interval_ms)
  {
    uint32_t steps = elapsed_happiness_ms / happiness_interval_ms;
    Pet_DecreaseValue(&pet.happiness, steps);
    pet.last_happiness_decay_ms += steps * happiness_interval_ms;
  }

  Pet_CheckTerminalState(now_ms);

  /* A terminal state takes precedence over ending a feeding/play/talk state. */
  if ((pet.state != PET_STATE_DEAD) && (pet.state != PET_STATE_RAN_AWAY) &&
      (pet.state != PET_STATE_IDLE) &&
      ((now_ms - pet.state_start_ms) >= PET_ACTION_DURATION_MS))
  {
    pet.state = PET_STATE_IDLE;
    pet.state_start_ms = now_ms;
  }
}

bool Pet_DoAction(PetAction action, uint32_t now_ms)
{
  if ((pet.state == PET_STATE_DEAD) || (pet.state == PET_STATE_RAN_AWAY))
  {
    /* Terminal states ignore all actions until the user restarts the pet. */
    return false;
  }

  /*
   * Feeding is repeatable: each B1 tap adds fullness and extends the eating
   * animation, matching the menu-flow specification.
   */
  if ((action == PET_ACTION_FEED) && (pet.state == PET_STATE_FEEDING))
  {
    Pet_IncreaseValue(&pet.fullness, PET_FEED_FULLNESS_CHANGE);
    pet.state_start_ms = now_ms;
    return true;
  }

  if (pet.state != PET_STATE_IDLE)
  {
    /* Other actions wait for the current animation to finish. */
    return false;
  }

  switch (action)
  {
    case PET_ACTION_FEED:
      /* Every feed tap makes the pet fuller, up to the 100-point limit. */
      Pet_IncreaseValue(&pet.fullness, PET_FEED_FULLNESS_CHANGE);
      pet.state = PET_STATE_FEEDING;
      break;

    case PET_ACTION_PLAY:
      /* Retained for a simple sensor-driven play interaction. */
      Pet_IncreaseValue(&pet.happiness, PET_PLAY_HAPPINESS_CHANGE);
      pet.state = PET_STATE_PLAYING;
      break;

    case PET_ACTION_TALK:
      /* A loud-enough microphone event maps to talk and raises happiness. */
      Pet_IncreaseValue(&pet.happiness, PET_TALK_HAPPINESS_CHANGE);
      pet.state = PET_STATE_TALKING;
      break;

    default:
      return false;
  }

  pet.state_start_ms = now_ms;
  /* The UI uses this time to synchronize its animation with the action. */
  return true;
}

bool Pet_ApplyGameResult(bool won, uint32_t now_ms)
{
  if ((pet.state == PET_STATE_DEAD) ||
      (pet.state == PET_STATE_RAN_AWAY))
  {
    return false;
  }

  if (won)
  {
    Pet_IncreaseValue(
        &pet.happiness,
        PET_GAME_HAPPINESS_CHANGE);
  }
  else
  {
    Pet_DecreaseValue(
        &pet.happiness,
        PET_GAME_HAPPINESS_CHANGE);
  }

  Pet_CheckTerminalState(now_ms);

  if ((pet.state != PET_STATE_DEAD) &&
      (pet.state != PET_STATE_RAN_AWAY))
  {
    pet.state = PET_STATE_IDLE;
    pet.state_start_ms = now_ms;
  }

  return true;
}

void Pet_SetPaused(bool paused, uint32_t now_ms)
{
  if (pet.paused == paused)
  {
    return;
  }

  pet.paused = paused;

  /*
   * Reset both timestamps so time spent in the game is discarded instead
   * of being applied as delayed decay after leaving.
   */
  pet.last_fullness_decay_ms = now_ms;
  pet.last_happiness_decay_ms = now_ms;
}

void Pet_SetDemoMode(bool enabled, uint32_t now_ms)
{
  if (pet.demo_mode != enabled)
  {
    pet.demo_mode = enabled;
    /* Start each meter's new decay rate from the moment of the mode change. */
    pet.last_fullness_decay_ms = now_ms;
    pet.last_happiness_decay_ms = now_ms;
  }
}

bool Pet_IsDemoMode(void)
{
  return pet.demo_mode;
}

void Pet_Reset(uint32_t now_ms)
{
  /* Reset currently restores the same healthy starting values as power-on. */
  Pet_Init(now_ms);
}

uint8_t Pet_GetFullness(void)
{
  return pet.fullness;
}

uint8_t Pet_GetHappiness(void)
{
  return pet.happiness;
}

PetState Pet_GetState(void)
{
  return pet.state;
}

uint32_t Pet_GetStateStartMs(void)
{
  return pet.state_start_ms;
}
