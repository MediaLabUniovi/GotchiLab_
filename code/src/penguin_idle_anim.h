#pragma once
#include <stdint.h>

#define PENGUIN_IDLE_WIDTH 128
#define PENGUIN_IDLE_HEIGHT 64
#define PENGUIN_IDLE_FPS 5
#define PENGUIN_IDLE_SECONDS 3
#define PENGUIN_IDLE_FRAMES 15
#define PENGUIN_IDLE_FRAME_SIZE 1024

extern const uint8_t penguin_idle_anim[PENGUIN_IDLE_FRAMES][PENGUIN_IDLE_FRAME_SIZE];
extern const int penguin_idle_offsets[PENGUIN_IDLE_FRAMES];