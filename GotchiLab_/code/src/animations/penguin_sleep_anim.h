#pragma once
#include <stdint.h>

#define PENGUIN_SLEEP_WIDTH 128
#define PENGUIN_SLEEP_HEIGHT 64
#define PENGUIN_SLEEP_FPS 5
#define PENGUIN_SLEEP_SECONDS 3
#define PENGUIN_SLEEP_FRAMES 15
#define PENGUIN_SLEEP_FRAME_SIZE 1024

extern const uint8_t penguin_sleep_anim[PENGUIN_SLEEP_FRAMES][PENGUIN_SLEEP_FRAME_SIZE];
extern const int penguin_sleep_offsets[PENGUIN_SLEEP_FRAMES];
