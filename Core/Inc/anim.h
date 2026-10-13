/**
 ******************************************************************************
 * @file    anim.h
 * @brief   Nonblocking frame-by-frame animation player.
 *
 * An animation is a draw function plus a frame count and a time per frame.
 * Anim_Update() is called on every main-loop pass; it only draws when the
 * current frame's time is up, and returns immediately otherwise, so input
 * and other work keep running while an animation plays.
 ******************************************************************************
 */
#ifndef ANIM_H
#define ANIM_H

#include <stdint.h>

/* Draws frame number `frame` with its top-left corner at (x, y). This is
 * where a sprite gets drawn, e.g. with ILI9341_DrawImageScaled(). */
typedef void (*AnimDrawFn)(uint8_t frame, uint16_t x, uint16_t y);

typedef struct
{
  AnimDrawFn draw;
  uint8_t    frame_count;
  uint16_t   frame_ms;     /* how long each frame stays on screen */
  uint8_t    loop;         /* 1 = repeat forever, 0 = play once */
} Animation;

typedef struct
{
  const Animation *anim;   /* NULL when stopped */
  uint16_t x;
  uint16_t y;
  uint8_t  frame;
  uint32_t frame_start_ms;
} AnimPlayer;

/* Starts `anim` at (x, y) and draws its first frame right away. */
void Anim_Start(AnimPlayer *player, const Animation *anim, uint16_t x, uint16_t y, uint32_t now_ms);

/* Draws the next frame if the current one has been shown for frame_ms.
 * Returns 1 exactly once when a play-once animation has finished (its last
 * frame stays on screen), otherwise 0. */
uint8_t Anim_Update(AnimPlayer *player, uint32_t now_ms);

/* Stops the animation (whatever is on screen stays there). */
void Anim_Stop(AnimPlayer *player);

/* Returns the animation currently playing, or NULL. */
const Animation *Anim_Current(const AnimPlayer *player);

#endif /* ANIM_H */
