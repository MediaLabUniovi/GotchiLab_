#pragma once
#include <stdint.h>

#define PENGUIN_IDLE_EGG_WIDTH 128
#define PENGUIN_IDLE_EGG_HEIGHT 64
#define PENGUIN_IDLE_EGG_FPS 5
#define PENGUIN_IDLE_EGG_SECONDS 3
#define PENGUIN_IDLE_EGG_FRAMES 15
#define PENGUIN_IDLE_EGG_FRAME_SIZE 1024

extern const uint8_t penguin_idle_egg_anim[PENGUIN_IDLE_EGG_FRAMES][PENGUIN_IDLE_EGG_FRAME_SIZE];
extern const int penguin_idle_egg_offsets[PENGUIN_IDLE_EGG_FRAMES];
