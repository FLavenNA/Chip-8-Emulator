#include "sdl.h"
#include "app.h"
#include "chip8.h"

bool init_sdl(sdl_t *sdl, config_t *config)
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

    SDL_memset(&sdl->want, 0, sizeof(sdl->want)); /* or SDL_zero(want) */
    // Init audio stuff
    sdl->want = (SDL_AudioSpec) {
        .freq = 44100,              // 44100hz "CD" quality
        .format = SDL_AUDIO_S16LE,  // Signed 16 bit little endian
        .channels = 1,              // Mono, 1 channel
    };

    sdl->stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &sdl->want, NULL, NULL);

    if (!sdl->stream) {
        SDL_Log("Could not create SDL Audio Stream %s\n", SDL_GetError());
        return false;
    }

    if ((sdl->want.freq != sdl->want.freq) ||
        (sdl->want.format != sdl->want.format) ||
        (sdl->want.channels != sdl->want.channels))
    {
        SDL_Log("Could not get desired Audio Spec\n");
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
void update_screen(const sdl_t sdl, const config_t *config, chip8_t *chip8) {
    SDL_FRect rect = {
        .x = 0,
        .y = 0,
        .w = (float)config->scale_factor,
        .h = (float)config->scale_factor,
    };

    // Grap background color values to draw outlines
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
            if (chip8->pixel_color[i] != config->foreground_color) {
                // Lerp towards foreground color
                chip8->pixel_color[i] = color_lerp(chip8->pixel_color[i], 
                                                   config->foreground_color, 
                                                   config->color_lerp_rate); 
            }

            const uint8_t r = (chip8->pixel_color[i] >> 24) & 0xFF;
            const uint8_t g = (chip8->pixel_color[i] >> 16) & 0xFF;
            const uint8_t b = (chip8->pixel_color[i] >> 8) & 0xFF;
            const uint8_t a = (chip8->pixel_color[i] >> 0) & 0xFF;

            SDL_SetRenderDrawColor(sdl.renderer, r, g, b, a);
            SDL_RenderFillRect(sdl.renderer, &rect);

            // TODO: Move this outside if/else
            // if user requested drawing pixel outlines
            if(config->pixel_outlines) {
                SDL_SetRenderDrawColor(sdl.renderer, bg_r, bg_g, bg_b, bg_a);
                SDL_RenderRect(sdl.renderer, &rect);
            }

        } else {
            // If not draw background color
            if (chip8->pixel_color[i] != config->foreground_color) {
                // Lerp towards background color
                chip8->pixel_color[i] = color_lerp(chip8->pixel_color[i], 
                                                   config->background_color, 
                                                   config->color_lerp_rate); 
            }

            const uint8_t r = (chip8->pixel_color[i] >> 24) & 0xFF;
            const uint8_t g = (chip8->pixel_color[i] >> 16) & 0xFF;
            const uint8_t b = (chip8->pixel_color[i] >> 8) & 0xFF;
            const uint8_t a = (chip8->pixel_color[i] >> 0) & 0xFF;

            SDL_SetRenderDrawColor(sdl.renderer, r, g, b, a);
            SDL_RenderFillRect(sdl.renderer, &rect);
        }
    }

    SDL_RenderPresent(sdl.renderer);
}

void final_cleanup(const sdl_t sdl)
{
    SDL_DestroyRenderer(sdl.renderer);
    SDL_DestroyWindow(sdl.window);
    SDL_DestroyAudioStream(sdl.stream);
    SDL_Quit();
}

uint32_t color_lerp(const uint32_t start_color, const uint32_t end_color, const float t) {
    const uint8_t s_r = (uint8_t)((start_color >> 24) & 0xFF);
    const uint8_t s_g = (uint8_t)((start_color >> 16) & 0xFF);
    const uint8_t s_b = (uint8_t)((start_color >> 8) & 0xFF);
    const uint8_t s_a = (uint8_t)((start_color >> 0) & 0xFF);

    const uint8_t e_r = (uint8_t)((end_color >> 24) & 0xFF);
    const uint8_t e_g = (uint8_t)((end_color >> 16) & 0xFF);
    const uint8_t e_b = (uint8_t)((end_color >> 8) & 0xFF);
    const uint8_t e_a = (uint8_t)((end_color >> 0) & 0xFF);

    const uint8_t ret_r = (uint8_t)(((1-t) * s_r) + (t * e_r));
    const uint8_t ret_g = (uint8_t)(((1-t) * s_g) + (t * e_g));
    const uint8_t ret_b = (uint8_t)(((1-t) * s_b) + (t * e_b));
    const uint8_t ret_a = (uint8_t)(((1-t) * s_a) + (t * e_a));

    return (ret_r << 24) | (ret_g << 16) | (ret_b << 8) | ret_a;
}