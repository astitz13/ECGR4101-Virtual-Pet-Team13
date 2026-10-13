/**
 ******************************************************************************
 * @file    sound.h
 * @brief   Sound cues for the virtual pet: one named cue per pet event, each
 *          mapped to an audio clip played through audio.c.
 *
 * To give a cue a real sound: convert the audio file with
 * Assets/convert_audio.py, then put the generated array and its sample count
 * into that cue's entry of the table in sound.c. Cues without a clip are
 * silent (and shown on screen as text while UI_SHOW_SOUND_CUES is 1 in ui.c).
 ******************************************************************************
 */
#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>

typedef enum
{
  SOUND_HAT_ON,         /* idle -> agent mode */
  SOUND_HAT_OFF,        /* agent mode -> back to idle */
  SOUND_FEED,           /* feed accepted by the pet logic */
  SOUND_HAPPY,          /* happiness increased (play or voice accepted) */
  SOUND_GAME_OVER,      /* dead or ran-away screen appears */
  SOUND_COUNT           /* number of cues (not a cue) */
} SoundCue;

/* Plays the clip for `cue`, replacing any sound already playing. Returns
 * immediately; playback continues in the background (audio.c interrupts).
 * Does nothing if the cue has no clip yet. */
void Sound_Play(SoundCue cue);

/* Stops whatever is playing. */
void Sound_Stop(void);

/* Short display name of a cue, e.g. "FEED" (used by the on-screen placeholder). */
const char *Sound_GetName(SoundCue cue);

#endif /* SOUND_H */
