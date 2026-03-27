#pragma once
#include <stdint.h>

#define PENGUIN_PET_WIDTH 128
#define PENGUIN_PET_HEIGHT 64
#define PENGUIN_PET_FPS 5
#define PENGUIN_PET_SECONDS 3
#define PENGUIN_PET_FRAMES 15
#define PENGUIN_PET_FRAME_SIZE 1024

extern const uint8_t penguin_pet_anim[PENGUIN_PET_FRAMES][PENGUIN_PET_FRAME_SIZE];
extern const int penguin_pet_offsets[PENGUIN_PET_FRAMES];
