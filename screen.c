#include "screen.h"

bool init_sdl(sdl_t *sdl, const config_t *config)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        SDL_Log("Could not initialize SDL3 : %s\n", SDL_GetError());
        return false;
    }

    sdl->window = SDL_CreateWindow("Chip-8 Emulator",
                                   config->window_width * config->scale_factor,
                                   config->window_height * config->scale_factor,
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

// Clear screen / SDL Window to background color
void clear_screen(const sdl_t sdl, const config_t *config)
{
    const uint8_t r = (config->background_color >> 24) & 0xFF;
    const uint8_t g = (config->background_color >> 16) & 0xFF;
    const uint8_t b = (config->background_color >> 8) & 0xFF;
    const uint8_t a = (config->background_color >> 0) & 0xFF;

    SDL_SetRenderDrawColor(sdl.renderer, r, g, b, a);
    SDL_RenderClear(sdl.renderer);
}

// Update window
void update_screen(const sdl_t sdl, const config_t *config, const chip8_t *chip8) {
    SDL_FRect rect = {
        .x = 0,
        .y = 0,
        .w = (float)config->scale_factor,
        .h = (float)config->scale_factor,
    };

    // Grap color values to draw
    const uint8_t fg_r = (config->foreground_color >> 24) & 0xFF;
    const uint8_t fg_g = (config->foreground_color >> 16) & 0xFF;
    const uint8_t fg_b = (config->foreground_color >> 8) & 0xFF;
    const uint8_t fg_a = (config->foreground_color >> 0) & 0xFF;

    const uint8_t bg_r = (config->background_color >> 24) & 0xFF;
    const uint8_t bg_g = (config->background_color >> 16) & 0xFF;
    const uint8_t bg_b = (config->background_color >> 8) & 0xFF;
    const uint8_t bg_a = (config->background_color >> 0) & 0xFF;

    // Loop through display pixels, draw a rectangle per pixel to the SDL window
    for(uint32_t i = 0; i < sizeof(chip8->display); i++) {
        // Translate 1D index i value to 2D X/Y coordinates
        // X = i % window width
        // Y = i / window width
        rect.x = (float)((i % config->window_width) * config->scale_factor);
        rect.y = (float)((i / config->window_width) * config->scale_factor);

        if(chip8->display[i]) {
            // If pixel is on, draw foreground color
            SDL_SetRenderDrawColor(sdl.renderer, fg_r, fg_g, fg_b, fg_a);
            SDL_RenderFillRect(sdl.renderer, &rect);

            // if user requested drawing pixel outlines
            if(config->pixel_outlines) {
                SDL_SetRenderDrawColor(sdl.renderer, bg_r, bg_g, bg_b, bg_a);
                SDL_RenderRect(sdl.renderer, &rect);
            }

        } else {
            // If not draw background color
            SDL_SetRenderDrawColor(sdl.renderer, bg_r, bg_g, bg_b, bg_a);
            SDL_RenderFillRect(sdl.renderer, &rect);
        }
    }

    SDL_RenderPresent(sdl.renderer);
}

void final_cleanup(const sdl_t sdl)
{
    SDL_DestroyRenderer(sdl.renderer);
    SDL_DestroyWindow(sdl.window);
    SDL_Quit();
}