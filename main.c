#include <time.h>
#include <SDL3/SDL.h>

#include "app.h"
#include "chip8.h"
#include "screen.h"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <rom_name>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    (void)argc;
    (void)argv;

    // Initialize config
    config_t config = {0};
    if (!set_config_from_args(&config, argc, argv))
        exit(EXIT_FAILURE);

    sdl_t sdl = {0};

    // Initialize SDL
    if (!init_sdl(&sdl, &config))
        exit(EXIT_FAILURE);

    chip8_t chip8 = {0};
    const char *rom_name = argv[1];
    
    if (!init_chip8(&chip8, rom_name))
        exit(EXIT_FAILURE);

    // Initial screen clear to background color
    clear_screen(sdl, &config);

    // Seed random number generator
    srand((uint32_t)time(NULL));

    // Main emulator loop
    while (chip8.state != QUIT)
    {
        // Handle user input
        handle_input(&chip8);

        if(chip8.state == PAUSED) continue;

        // get_time() before running instructions;
        const uint64_t start_frame_time = SDL_GetPerformanceCounter();

        // Emulate Chip-8 instructions for this emulator "frame" (60hz)
        for(uint32_t i = 0; i < config.insts_per_second / 60; i++)
            emulate_instruction(&chip8, &config);

        // get_time() elapsed after running instructions;
        const uint64_t end_frame_time = SDL_GetPerformanceCounter();

        // Delay for approximately 60hz/60fps (16.67ms) or actual time elapsed
        const double time_elapsed =  (double)((end_frame_time - start_frame_time) * 1000) / SDL_GetPerformanceFrequency();

        SDL_Delay((uint32_t)(16.67f > time_elapsed ? 16.67f - time_elapsed : 0));

        // Update window with changes every 60hz
        update_screen(sdl, &config, &chip8);

        // Update delay and sound timers every 60hz
        update_timers(&chip8);
    }

    // Final cleanup
    final_cleanup(sdl);

    exit(EXIT_SUCCESS);
}