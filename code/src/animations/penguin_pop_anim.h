#pragma once
#include <stdint.h>

#define PENGUIN_POP_WIDTH 128
#define PENGUIN_POP_HEIGHT 64
#define PENGUIN_POP_FPS 5
#define PENGUIN_POP_SECONDS 3
#define PENGUIN_POP_FRAMES 15
#define PENGUIN_POP_FRAME_SIZE 1024

extern const uint8_t penguin_pop_anim[PENGUIN_POP_FRAMES][PENGUIN_POP_FRAME_SIZE];
extern const int penguin_pop_offsets[PENGUIN_POP_FRAMES];
