/**
 ******************************************************************************
 * @file    anim.c
 * @brief   Nonblocking frame-by-frame animation player (see anim.h).
 ******************************************************************************
 */
#include "anim.h"
#include <stddef.h>

void Anim_Start(AnimPlayer *player, const Animation *anim, uint16_t x, uint16_t y, uint32_t now_ms)
{
  player->anim = anim;
  player->x = x;
  player->y = y;
  player->frame = 0U;
  player->frame_start_ms = now_ms;
  anim->draw(0U, x, y);
}

uint8_t Anim_Update(AnimPlayer *player, uint32_t now_ms)
{
  const Animation *anim = player->anim;

  if ((anim == NULL) || ((now_ms - player->frame_start_ms) < anim->frame_ms))
  {
    return 0U;   /* nothing playing, or current frame not finished yet */
  }

  if ((uint8_t)(player->frame + 1U) < anim->frame_count)
  {
    player->frame++;
  }
  else if (anim->loop)
  {
    player->frame = 0U;
  }
  else
  {
    player->anim = NULL;   /* play-once animation is done; last frame stays */
    return 1U;
  }

  player->frame_start_ms = now_ms;
  anim->draw(player->frame, player->x, player->y);
  return 0U;
}

void Anim_Stop(AnimPlayer *player)
{
  player->anim = NULL;
}

const Animation *Anim_Current(const AnimPlayer *player)
{
  return player->anim;
}
