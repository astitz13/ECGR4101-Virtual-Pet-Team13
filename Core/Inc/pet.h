/**
 * @file pet.h
 * @brief Hardware-independent virtual-pet state and action interface.
 */
#ifndef PET_H
#define PET_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Both meters stay in the inclusive range 0..100.
 * Fullness:  0 = empty and 100 = full.
 * Happiness: 0 = unhappy and 100 = very happy.
 */
#define PET_VALUE_MIN                 0U
#define PET_VALUE_MAX                 100U

#define PET_FULLNESS_DECAY_INTERVAL_MS 3000U
#define PET_HAPPINESS_DECAY_INTERVAL_MS 4000U
/* Faster rates used only when Pet_SetDemoMode(true, ...) is called. */
#define PET_DEMO_FULLNESS_DECAY_INTERVAL_MS 250U
#define PET_DEMO_HAPPINESS_DECAY_INTERVAL_MS 333U
#define PET_FEED_FULLNESS_CHANGE       10U
#define PET_PLAY_HAPPINESS_CHANGE     15U
#define PET_TALK_HAPPINESS_CHANGE     15U
#define PET_GAME_MAX_HAPPINESS_REWARD 15U
#define PET_ACTION_DURATION_MS        1500U

/* States that the UI can use to select a sprite or animation. */
typedef enum
{
  PET_STATE_IDLE,
  PET_STATE_FEEDING,
  PET_STATE_PLAYING,
  PET_STATE_TALKING,
  PET_STATE_DEAD,
  PET_STATE_RAN_AWAY
} PetState;

/* Actions requested by the menu or a sensor event. */
typedef enum
{
  PET_ACTION_FEED,
  PET_ACTION_PLAY,
  PET_ACTION_TALK
} PetAction;

/** Initialize a healthy, full, happy pet at the supplied system time. */
void Pet_Init(uint32_t now_ms);

/**
 * Advance timed decay and finish any temporary action state.
 * Call once per iteration of the nonblocking main loop.
 */
void Pet_Update(uint32_t now_ms);

/**
 * Apply an interaction. Returns false when the pet is unavailable, such as
 * during an animation or after death/runaway.
 */
bool Pet_DoAction(PetAction action, uint32_t now_ms);

/**
 * Award happiness after the Catch Game. A perfect score earns
 * PET_GAME_MAX_HAPPINESS_REWARD; lower scores earn a proportional amount.
 * Returns false if the pet has died or run away, or if max_score is zero.
 */
bool Pet_AwardGameHappiness(uint8_t score, uint8_t max_score, uint32_t now_ms);

/**
 * Enable or disable fast decay for a presentation/demo. Changing modes resets
 * the decay timers, so switching modes never applies a backlog of decay.
 */
void Pet_SetDemoMode(bool enabled, uint32_t now_ms);
bool Pet_IsDemoMode(void);

/** Restart the pet after it has died or run away. */
void Pet_Reset(uint32_t now_ms);

uint8_t Pet_GetFullness(void);
uint8_t Pet_GetHappiness(void);
PetState Pet_GetState(void);
uint32_t Pet_GetStateStartMs(void);

#endif /* PET_H */
