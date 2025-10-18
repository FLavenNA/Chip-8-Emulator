#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL3/SDL.h>

typedef struct
{
    SDL_Window *window;
    SDL_Renderer *renderer;
} sdl_t;

typedef struct
{
    uint32_t window_width;     // SDL Window Width
    uint32_t window_height;    // SDL Window Height
    uint32_t foreground_color; // Foreground color RGBA8888
    uint32_t background_color; // Background color RGBA8888
    uint32_t scale_factor;     // Chip8 pixel scale by
} config_t;

typedef enum
{
    QUIT = 0,
    RUNNING,
    PAUSED,
} emulator_state_t;

// Chip-8 machine object
typedef struct
{
    emulator_state_t state;
} chip8_t;

bool init_chip8(chip8_t *chip8)
{

    chip8->state = RUNNING; // Default state

    return true;
}

bool init_sdl(sdl_t *sdl, const config_t config)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        SDL_Log("Could not initialize SDL3 : %s\n", SDL_GetError());
        return false;
    }

    sdl->window = SDL_CreateWindow("Chip-8 Emulator",
                                   config.window_width * config.scale_factor,
                                   config.window_height * config.scale_factor,
                                   0);

    if (!sdl->window)
    {
        SDL_Log("Could not create SDL Window %s\n", SDL_GetError());
        return false;
    }

    sdl->renderer = SDL_CreateRenderer(sdl->window, NULL);

    if (!sdl->renderer)
    {
        SDL_Log("Could not create SDL renderer %s\n", SDL_GetError());
        return false;
    }

    return true;
}

// Set up initial emulator configuration from passed in arguments
bool set_config_from_args(config_t *config, int argc, char **argv)
{

    // Set defaults
    *config = (config_t){
        .window_width = 64,             // Chip-8 original X res
        .window_height = 32,            // Chip-8 original Y res
        .foreground_color = 0xFFFFFFFF, // Original color as white fg
        .background_color = 0xFFFF00FF, // Original color as black bg
        .scale_factor = 20,             // Default res will be 1280x640
    };

    // Override defaults
    for (int i = 1; i < argc; i++)
    {
        (void)argv[i];
    }

    return true;
}

// Clear screen / SDL Window to background color
void clear_screen(const sdl_t sdl, const config_t config)
{
    const uint8_t r = (config.background_color >> 24) & 0xFF;
    const uint8_t g = (config.background_color >> 16) & 0xFF;
    const uint8_t b = (config.background_color >> 8) & 0xFF;
    const uint8_t a = (config.background_color >> 0) & 0xFF;

    SDL_SetRenderDrawColor(sdl.renderer, r, g, b, a);
    SDL_RenderClear(sdl.renderer);
}

// Update window
void update_screen(const sdl_t sdl)
{
    SDL_RenderPresent(sdl.renderer);
}

void handle_input(chip8_t *chip8)
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_EVENT_QUIT:
                chip8->state = QUIT; // Will exit main emulator loop
                return;

            case SDL_EVENT_KEY_DOWN:
                switch (event.key.key)
                {
                    case SDLK_ESCAPE:
                        chip8->state = QUIT; // Exit window
                        return;
                    default:
                        break;
                }
                break;

            case SDL_EVENT_KEY_UP:
                break;

            default:
                break;
        }
    }
}

void final_cleanup(const sdl_t sdl)
{
    SDL_DestroyRenderer(sdl.renderer);
    SDL_DestroyWindow(sdl.window);
    SDL_Quit();
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    // Initialize config
    config_t config = {0};
    if (!set_config_from_args(&config, argc, argv))
        exit(EXIT_FAILURE);

    sdl_t sdl = {0};

    // Initialize SDL
    if (!init_sdl(&sdl, config))
        exit(EXIT_FAILURE);

    chip8_t chip8 = {0};
    if (!init_chip8(&chip8))
        exit(EXIT_FAILURE);

    // Main emulator loop
    while (chip8.state != QUIT)
    {
        // Handle user input
        handle_input(&chip8);

        // Initial screen clear to background color
        clear_screen(sdl, config);

        // if(chip8.state == PAUSED) continue;

        // get_time();
        // Emulate Chip-8 instructions
        // get_time() elapsed since last get_time();

        // Delay for approximately 60hz/60fps
        // SDL_Delay(16 - time elapsed);
        SDL_Delay(16);

        // Update
        update_screen(sdl);
    }

    // Final cleanup
    final_cleanup(sdl);

    exit(EXIT_SUCCESS);
}