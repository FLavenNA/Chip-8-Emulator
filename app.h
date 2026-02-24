#ifndef APP_H
#define APP_H

#include <stdint.h>
#include <stdbool.h>

#include "type_defs.h"

struct config
{
    uint32_t window_width;          // SDL Window Width
    uint32_t window_height;         // SDL Window Height
    uint32_t foreground_color;      // Foreground color RGBA8888
    uint32_t background_color;      // Background color RGBA8888
    uint32_t scale_factor;          // Chip8 pixel scale by
    bool pixel_outlines;            // Draw pixel outlines
    uint32_t insts_per_second;      // CHIP8 CPU "clock-rate" or hz
    uint32_t square_wave_freq;      // Frequency of square wave sound e.g 440hz for middle A
    uint32_t audio_sample_rate;     
    int16_t volume;                 // How loud is sound
    float color_lerp_rate;          // Amout to lerp colors by, between [0.1, 1.0]
    extension_t current_extension;  // Current extension support for e.g CHIP8 vs SUPERCHIP
};

enum emulator_state
{
    QUIT = 0,
    RUNNING,
    PAUSED,
};

enum extension
{
    CHIP8 = 0,
    SUPERCHIP,
    X0CHIP,
};

// Set up initial emulator configuration from passed in arguments
bool set_config_from_args(config_t *config, int argc, char **argv);

#endif