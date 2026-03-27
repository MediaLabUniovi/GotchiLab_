#pragma once
#include <stdint.h>

#define PENGUIN_BIRTH_WIDTH 128
#define PENGUIN_BIRTH_HEIGHT 64
#define PENGUIN_BIRTH_FPS 5
#define PENGUIN_BIRTH_SECONDS 3
#define PENGUIN_BIRTH_FRAMES 15
#define PENGUIN_BIRTH_FRAME_SIZE 1024

extern const uint8_t penguin_birth_anim[PENGUIN_BIRTH_FRAMES][PENGUIN_BIRTH_FRAME_SIZE];
extern const int penguin_birth_offsets[PENGUIN_BIRTH_FRAMES];
