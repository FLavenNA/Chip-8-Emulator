#include "app.h"

// Set up initial emulator configuration from passed in arguments
bool set_config_from_args(config_t *config, int argc, char **argv)
{

    // Set defaults
    *config = (config_t){
        .window_width = 64,             // Chip-8 original X res
        .window_height = 32,            // Chip-8 original Y res
        .foreground_color = 0xFFFFFFFF, // Original color as white fg
        .background_color = 0x00000000, // Original color as black bg
        .scale_factor = 20,             // Default res will be 1280x640
        .pixel_outlines = true,         // Draw pixel outlines by default
        .insts_per_second = 700,        // Number of instructions to emulate in 1 second (clock rate of CPU)
        .audio_sample_rate = 44100,     // CD quality
        .square_wave_freq = 440,        // 440hz for middle A
        .volume = 3000,                 // INT16_MAX would be max volume
        .color_lerp_rate = 0.7f          // Color lerp rate [0.1, 1.0]
    };

    // Override defaults
    for (int i = 1; i < argc; i++)
    {
        (void)argv[i];
    }

    return true;
}