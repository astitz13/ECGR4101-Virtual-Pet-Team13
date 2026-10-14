#include "game.h"
#include "ili9341.h"
#include "imported_images.h"
#include "mpu6050.h"

#define GAME_BG_COLOR              ILI9341_COLOR_BLACK
#define GAME_TEXT_COLOR            ILI9341_COLOR_WHITE

#define GAME_BODY_Y                60U
#define GAME_FIELD_TOP_Y           110U

#define GAME_LANE_COUNT            3U
#define GAME_LANE_WIDTH            (ILI9341_WIDTH / GAME_LANE_COUNT)

#define GAME_PET_SCALE             2U
#define GAME_OBJECT_SCALE          2U

#define GAME_PET_WIDTH             (IMG_GAME_RUN_0_WIDTH * GAME_PET_SCALE)
#define GAME_PET_HEIGHT            (IMG_GAME_RUN_0_HEIGHT * GAME_PET_SCALE)

#define GAME_OBJECT_WIDTH          (IMG_FISH_0_WIDTH * GAME_OBJECT_SCALE)
#define GAME_OBJECT_HEIGHT         (IMG_FISH_0_HEIGHT * GAME_OBJECT_SCALE)

#define GAME_PET_Y                 244U
#define GAME_OBJECT_START_Y        GAME_FIELD_TOP_Y

#define GAME_SENSOR_INTERVAL_MS    40U
#define GAME_FALL_INTERVAL_MS      80U
#define GAME_RESPAWN_DELAY_MS      200U
#define GAME_FALL_STEP_PX          8U

#define GAME_TILT_THRESHOLD        0.30f
#define GAME_TILT_FILTER           0.25f
#define GAME_CALIBRATION_SAMPLES   16U

/*
 * Change this to 1 if tilting left moves Perry right and vice versa.
 * If the wrong accelerometer axis is being used, change data.acc_x to
 * data.acc_y inside Game_UpdateTilt().
 */
#define GAME_REVERSE_TILT          0

#define GAME_FISH_TARGET           10U
#define GAME_BOMB_LIMIT            3U

typedef enum
{
  GAME_OBJECT_FISH,
  GAME_OBJECT_BOMB
} GameObjectType;

typedef struct
{
    uint8_t pet_lane;
    uint8_t object_lane;

    uint8_t fish_count;
    uint8_t bomb_count;

    uint8_t pet_frame;
    uint8_t object_frame;

    uint16_t object_y;

    GameObjectType object_type;
    GameResult result;

    bool object_active;
    bool waiting_for_spawn;
    bool finished;
    bool result_reported;
    
    float tilt_zero;
    float filtered_tilt;

    uint32_t last_sensor_ms;
    uint32_t last_fall_ms;
    uint32_t object_finished_ms;
    uint32_t random_state;
} GameState;

static GameState game;

static uint16_t Game_GetPetX(uint8_t lane)
{
  return (uint16_t)(
      lane * GAME_LANE_WIDTH +
      (GAME_LANE_WIDTH - GAME_PET_WIDTH) / 2U);
}

static uint16_t Game_GetObjectX(uint8_t lane)
{
  return (uint16_t)(
      lane * GAME_LANE_WIDTH +
      (GAME_LANE_WIDTH - GAME_OBJECT_WIDTH) / 2U);
}

static void Game_DrawImage(
    uint16_t x,
    uint16_t y,
    uint16_t width,
    uint16_t height,
    const image_pixel_t *pixels,
    uint16_t scale)
{
  ILI9341_DrawImageScaled(
      x,
      y,
      width,
      height,
      (image_pixel_t *)pixels,
      scale);
}

static void Game_DrawHUD(void)
{
  char score_text[] = "FISH: 00/10  BOMBS: 0/3";

  score_text[6] = (char)('0' + game.fish_count / 10U);
  score_text[7] = (char)('0' + game.fish_count % 10U);
  score_text[20] = (char)('0' + game.bomb_count);

  ILI9341_FillRect(
      0U,
      86U,
      ILI9341_WIDTH,
      12U,
      GAME_BG_COLOR);

  ILI9341_DrawString(
      24U,
      86U,
      score_text,
      GAME_TEXT_COLOR,
      GAME_BG_COLOR,
      1U);
}

static void Game_DrawBackground(void)
{
  ILI9341_FillRect(
      0U,
      GAME_BODY_Y,
      ILI9341_WIDTH,
      ILI9341_HEIGHT - GAME_BODY_Y,
      GAME_BG_COLOR);

  ILI9341_DrawString(
      60U,
      64U,
      "TILT CATCH",
      ILI9341_COLOR_CYAN,
      GAME_BG_COLOR,
      2U);

  Game_DrawHUD();

  /* Lane separators. */
  ILI9341_DrawVLine(
      GAME_LANE_WIDTH,
      102U,
      206U,
      ILI9341_COLOR_GRAY);

  ILI9341_DrawVLine(
      GAME_LANE_WIDTH * 2U,
      102U,
      206U,
      ILI9341_COLOR_GRAY);
}

static void Game_ClearPet(uint8_t lane)
{
  ILI9341_FillRect(
      Game_GetPetX(lane),
      GAME_PET_Y,
      GAME_PET_WIDTH,
      GAME_PET_HEIGHT,
      GAME_BG_COLOR);
}

static void Game_DrawNormalPet(void)
{
  const image_pixel_t *sprite =
      (game.pet_frame == 0U)
          ? IMG_GAME_RUN_0
          : IMG_GAME_RUN_1;

  Game_ClearPet(game.pet_lane);

  Game_DrawImage(
      Game_GetPetX(game.pet_lane),
      GAME_PET_Y,
      IMG_GAME_RUN_0_WIDTH,
      IMG_GAME_RUN_0_HEIGHT,
      sprite,
      GAME_PET_SCALE);
}

static void Game_DrawPetFeedback(const image_pixel_t *sprite)
{
  Game_ClearPet(game.pet_lane);

  Game_DrawImage(
      Game_GetPetX(game.pet_lane),
      GAME_PET_Y,
      IMG_GAME_CATCH_WIDTH,
      IMG_GAME_CATCH_HEIGHT,
      sprite,
      GAME_PET_SCALE);
}

static void Game_ClearObject(void)
{
  ILI9341_FillRect(
      Game_GetObjectX(game.object_lane),
      game.object_y,
      GAME_OBJECT_WIDTH,
      GAME_OBJECT_HEIGHT,
      GAME_BG_COLOR);
}

static void Game_DrawObject(void)
{
  const image_pixel_t *sprite;

  if (game.object_type == GAME_OBJECT_BOMB)
  {
    sprite = IMG_BOMB_0;
  }
  else
  {
    sprite = (game.object_frame == 0U)
        ? IMG_FISH_0
        : IMG_FISH_1;
  }

  Game_DrawImage(
      Game_GetObjectX(game.object_lane),
      game.object_y,
      IMG_FISH_0_WIDTH,
      IMG_FISH_0_HEIGHT,
      sprite,
      GAME_OBJECT_SCALE);
}

static uint32_t Game_GetRandom(void)
{
  game.random_state =
      game.random_state * 1664525UL + 1013904223UL;

  return game.random_state;
}

static void Game_SpawnObject(uint32_t now_ms)
{
  uint32_t random_value = Game_GetRandom();

  game.object_lane =
      (uint8_t)(random_value % GAME_LANE_COUNT);

  /*
   * Approximately 70% of objects are fish and 30% are bombs.
   */
  game.object_type =
      (((random_value >> 8U) % 10U) < 7U)
          ? GAME_OBJECT_FISH
          : GAME_OBJECT_BOMB;

  game.object_y = GAME_OBJECT_START_Y;
  game.object_frame = 0U;
  game.object_active = true;
  game.waiting_for_spawn = false;
  game.last_fall_ms = now_ms;

  Game_DrawNormalPet();
  Game_DrawObject();
}

static void Game_DrawResult(GameResult result)
{
  ILI9341_FillRect(
      0U,
      GAME_BODY_Y,
      ILI9341_WIDTH,
      ILI9341_HEIGHT - GAME_BODY_Y,
      GAME_BG_COLOR);

  if (result == GAME_RESULT_WIN)
  {
    ILI9341_DrawString(
        48U,
        100U,
        "YOU WIN!",
        ILI9341_COLOR_GREEN,
        GAME_BG_COLOR,
        3U);

    ILI9341_DrawString(
        48U,
        160U,
        "10 FISH CAUGHT",
        GAME_TEXT_COLOR,
        GAME_BG_COLOR,
        2U);

    ILI9341_DrawString(
        54U,
        190U,
        "HAPPINESS +10",
        ILI9341_COLOR_GREEN,
        GAME_BG_COLOR,
        2U);
  }
  else
  {
    ILI9341_DrawString(
        39U,
        100U,
        "YOU LOSE!",
        ILI9341_COLOR_RED,
        GAME_BG_COLOR,
        3U);

    ILI9341_DrawString(
        48U,
        160U,
        "3 BOMBS HIT",
        GAME_TEXT_COLOR,
        GAME_BG_COLOR,
        2U);

    ILI9341_DrawString(
        54U,
        190U,
        "HAPPINESS -10",
        ILI9341_COLOR_RED,
        GAME_BG_COLOR,
        2U);
  }

  ILI9341_DrawString(
      42U,
      260U,
      "CENTER: AGAIN",
      GAME_TEXT_COLOR,
      GAME_BG_COLOR,
      2U);

  ILI9341_DrawString(
      66U,
      290U,
      "LEFT: BACK",
      ILI9341_COLOR_GRAY,
      GAME_BG_COLOR,
      1U);
}

static void Game_Finish(GameResult result)
{
  game.finished = true;
  game.object_active = false;
  game.waiting_for_spawn = false;
  game.result = result;

  Game_DrawResult(result);
}

static void Game_CalibrateTilt(void)
{
  MPU6050_data_t data;
  float sum = 0.0f;
  float sample;
  uint8_t i;

  /*
   * Take 16 samples before gameplay begins.
   * Hold the board centered while entering the game.
   */
  for (i = 0U; i < GAME_CALIBRATION_SAMPLES; i++)
  {
    MPU6050_getParsedData(&data);
    sample = data.acc_y;

#if GAME_REVERSE_TILT
    sample = -sample;
#endif

    sum += sample;

    /*
     * MPU6050 is configured for a new sample approximately every 4 ms.
     */
    HAL_Delay(5U);
  }

  game.tilt_zero = sum / GAME_CALIBRATION_SAMPLES;
  game.filtered_tilt = 0.0f;
}

static void Game_UpdateTilt(void)
{
  MPU6050_data_t data;
  float raw_tilt;
  float tilt;
  uint8_t new_lane;

  MPU6050_getParsedData(&data);
  raw_tilt = data.acc_y;

#if GAME_REVERSE_TILT
  raw_tilt = -raw_tilt;
#endif

  /* Make the calibrated center equal to zero. */
  tilt = raw_tilt - game.tilt_zero;

  /* Smooth the sensor reading. */
  game.filtered_tilt +=
      (tilt - game.filtered_tilt) * GAME_TILT_FILTER;

  if (game.filtered_tilt < -GAME_TILT_THRESHOLD)
  {
    new_lane = 0U;
  }
  else if (game.filtered_tilt > GAME_TILT_THRESHOLD)
  {
    new_lane = 2U;
  }
  else
  {
    new_lane = 1U;
  }

  if (new_lane != game.pet_lane)
  {
    Game_ClearPet(game.pet_lane);

    game.pet_lane = new_lane;
    game.pet_frame ^= 1U;

    Game_DrawNormalPet();
  }
}

static void Game_AdvanceObject(uint32_t now_ms)
{
  bool caught;

  Game_ClearObject();

  game.object_y =
      (uint16_t)(game.object_y + GAME_FALL_STEP_PX);

  /*
   * The object reaches Perry when its bottom edge reaches the top of
   * Perry's sprite.
   */
  if ((game.object_y + GAME_OBJECT_HEIGHT) >= GAME_PET_Y)
  {
    game.object_active = false;
    caught = (game.object_lane == game.pet_lane);

    if (caught)
    {
      if (game.object_type == GAME_OBJECT_FISH)
      {
        if (game.fish_count < GAME_FISH_TARGET)
        {
          game.fish_count++;
        }

        Game_DrawPetFeedback(IMG_GAME_CATCH);
      }
      else
      {
        if (game.bomb_count < GAME_BOMB_LIMIT)
        {
          game.bomb_count++;
        }

        Game_DrawPetFeedback(IMG_GAME_HIT);
      }

      Game_DrawHUD();
    }

    if (game.fish_count >= GAME_FISH_TARGET)
    {
      Game_Finish(GAME_RESULT_WIN);
      return;
    }

    if (game.bomb_count >= GAME_BOMB_LIMIT)
    {
      Game_Finish(GAME_RESULT_LOSS);
      return;
    }

    game.waiting_for_spawn = true;
    game.object_finished_ms = now_ms;
    return;
  }

  game.object_frame ^= 1U;
  Game_DrawObject();
}

void Game_Start(uint32_t now_ms)
{
    game.pet_lane = 1U;
    game.object_lane = 1U;

    game.fish_count = 0U;
    game.bomb_count = 0U;

    game.pet_frame = 0U;
    game.object_frame = 0U;

    game.object_y = GAME_OBJECT_START_Y;

    game.object_active = false;
    game.waiting_for_spawn = false;
    game.finished = false;
    game.result_reported = false;

    game.tilt_zero = 0.0f;
    game.filtered_tilt = 0.0f;

    Game_CalibrateTilt();

    game.result = GAME_RESULT_NONE;

    game.last_sensor_ms = now_ms;
    game.last_fall_ms = now_ms;
    game.object_finished_ms = now_ms;

    game.random_state = now_ms ^ 0xA5A55A5AUL;

    Game_DrawBackground();
    Game_DrawNormalPet();
    Game_SpawnObject(now_ms);
}

GameResult Game_Update(uint32_t now_ms)
{
  if (game.finished)
  {
    if (!game.result_reported)
    {
      game.result_reported = true;
      return game.result;
    }

    return GAME_RESULT_NONE;
  }

  if ((now_ms - game.last_sensor_ms) >=
      GAME_SENSOR_INTERVAL_MS)
  {
    game.last_sensor_ms = now_ms;
    Game_UpdateTilt();
  }

  if (game.waiting_for_spawn &&
      ((now_ms - game.object_finished_ms) >=
       GAME_RESPAWN_DELAY_MS))
  {
    Game_SpawnObject(now_ms);
  }

  if (game.object_active &&
      ((now_ms - game.last_fall_ms) >=
       GAME_FALL_INTERVAL_MS))
  {
    game.last_fall_ms = now_ms;
    Game_AdvanceObject(now_ms);
  }

  if (game.finished && !game.result_reported)
  {
    game.result_reported = true;
    return game.result;
  }

  return GAME_RESULT_NONE;
}

bool Game_IsFinished(void)
{
  return game.finished;
}