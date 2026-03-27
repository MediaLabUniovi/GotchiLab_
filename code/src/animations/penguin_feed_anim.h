#pragma once
#include <stdint.h>

#define PENGUIN_FEED_WIDTH 128
#define PENGUIN_FEED_HEIGHT 64
#define PENGUIN_FEED_FPS 5
#define PENGUIN_FEED_SECONDS 3
#define PENGUIN_FEED_FRAMES 15
#define PENGUIN_FEED_FRAME_SIZE 1024

extern const uint8_t penguin_feed_anim[PENGUIN_FEED_FRAMES][PENGUIN_FEED_FRAME_SIZE];
extern const int penguin_feed_offsets[PENGUIN_FEED_FRAMES];
