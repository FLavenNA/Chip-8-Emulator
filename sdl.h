#ifndef SDL_H
#define SDL_H

#include <SDL3/SDL.h>

#include "type_defs.h"

struct sdl
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_AudioSpec want;
    SDL_AudioStream *stream;
};

bool init_sdl(sdl_t *sdl, config_t *config);
void clear_screen(const sdl_t sdl, const config_t *config);
void update_screen(const sdl_t sdl, const config_t *config, chip8_t *chip8);
void final_cleanup(const sdl_t sdl);

// Helper functions
uint32_t color_lerp(const uint32_t start_color, const uint32_t end_color, const float t);
#endif