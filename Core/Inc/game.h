#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
  GAME_RESULT_NONE,
  GAME_RESULT_WIN,
  GAME_RESULT_LOSS
} GameResult;

/* Starts or restarts the tilt-controlled catch game. */
void Game_Start(uint32_t now_ms);

/* Updates tilt, falling objects, collisions, and score. */
GameResult Game_Update(uint32_t now_ms);

/* Returns true after collecting 10 fish or hitting 3 bombs. */
bool Game_IsFinished(void);

#endif /* GAME_H */