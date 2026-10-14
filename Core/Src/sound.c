/**
 ******************************************************************************
 * @file    sound.c
 * @brief   Sound cues for the virtual pet (see sound.h).
 ******************************************************************************
 */
#include "sound.h"
#include "audio.h"
#include <stddef.h>
#include "imported_audio.h"

typedef struct
{
  const int8_t *samples;   /* clip data from convert_audio.py, NULL = no clip yet */
  size_t        count;     /* number of samples in the clip */
  const char   *name;      /* short name shown by the on-screen placeholder */
} SoundClip;

/* One entry per cue (see SoundCue in sound.h).
 *
 * To add a sound, e.g. for feeding:
 *   1. Add the file to SOUND_LIST in Assets/convert_audio.py, named e.g.
 *      "AUDIO_FEED", and run the script (it updates imported_audio.h/.c).
 *   2. Replace { NULL, 0U, "FEED" } below with
 *      { AUDIO_FEED, AUDIO_FEED_SAMPLES, "FEED" }
 *      and add #include "imported_audio.h" at the top of this file.
 * Keep clips short: about 16 KB of Flash per second of audio (16 kHz). */
static const SoundClip sound_clips[SOUND_COUNT] =
{
  [SOUND_HAT_ON] = {
    .samples = AUDIO_HAT_ON,
    .count = AUDIO_HAT_ON_SAMPLES,
    .name = "HAT ON"
  },

  [SOUND_HAT_OFF] = {
    .samples = AUDIO_HAT_OFF,
    .count = AUDIO_HAT_OFF_SAMPLES,
    .name = "HAT OFF"
  },

  [SOUND_FEED] = {
    .samples = AUDIO_FEED,
    .count = AUDIO_FEED_SAMPLES,
    .name = "FEED"
  },

  [SOUND_HAPPY] = {
    .samples = AUDIO_HAPPY,
    .count = AUDIO_HAPPY_SAMPLES,
    .name = "HAPPY"
  },

  [SOUND_GAME_OVER] = {
    .samples = AUDIO_GAME_OVER,
    .count = AUDIO_GAME_OVER_SAMPLES,
    .name = "GAME OVER"
  },
};
void Sound_Play(SoundCue cue)
{
  if ((cue < SOUND_COUNT) && (sound_clips[cue].samples != NULL) && (sound_clips[cue].count > 0U))
  {
    /* startAudio() only reads the samples; the cast drops const for its signature */
    startAudio((int8_t *)sound_clips[cue].samples, sound_clips[cue].count);
  }
}

void Sound_Stop(void)
{
  stopAudio();
}

const char *Sound_GetName(SoundCue cue)
{
  return (cue < SOUND_COUNT) ? sound_clips[cue].name : "";
}
