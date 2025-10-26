#ifndef APP_H
#define APP_H

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint32_t window_width;     // SDL Window Width
    uint32_t window_height;    // SDL Window Height
    uint32_t foreground_color; // Foreground color RGBA8888
    uint32_t background_color; // Background color RGBA8888
    uint32_t scale_factor;     // Chip8 pixel scale by
    bool pixel_outlines;       // Draw pixel outlines
    uint32_t insts_per_second;  // CHIP8 CPU "clock-rate" or hz
} config_t;

typedef enum
{
    QUIT = 0,
    RUNNING,
    PAUSED,
} emulator_state_t;

// Set up initial emulator configuration from passed in arguments
bool set_config_from_args(config_t *config, int argc, char **argv);

#endif