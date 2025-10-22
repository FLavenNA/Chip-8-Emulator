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
    bool pixel_outlines;       // Draw pixel outlines
} config_t;

typedef enum
{
    QUIT = 0,
    RUNNING,
    PAUSED,
} emulator_state_t;

// TODO Maybe make this bit fields if you can guarante ?
typedef struct
{
    uint16_t opcode;
    uint16_t NNN; // 12 bit address/constant
    uint8_t NN;   // 8 bit constant
    uint8_t N;    // 4 bit constant
    uint8_t X;    // 4 bit register identifier
    uint8_t Y;    // 4 bit register identifier
} instruction_t;

// Chip-8 machine object
typedef struct
{
    emulator_state_t state;
    uint8_t ram[4096];
    bool display[64 * 32]; // Emulator original resolution pixels
    uint16_t stack[12];    // Subroutine stack
    uint16_t *stack_ptr;
    uint8_t V[16];        // Data registers V0 - VF
    uint16_t I;           // Index register
    uint16_t PC;          // Program Counter
    uint8_t sound_timer;  // Decrement at 60hz and plays tone when > 0z
    uint8_t delay_timer;  // Decrement at 60hz when > 0
    bool keypad[16];      // Hexadecimal keypad 0x0 - 0xF
    const char *rom_name; // Currently running ROM
    instruction_t inst;   // Currently executing instruction
} chip8_t;

bool init_chip8(chip8_t *chip8, const char rom_name[])
{
    const uint32_t entry_point = 0x200; // Chip8 roms will be loaded to 0x200
    const uint8_t font[] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };

    // Load font
    memcpy(&chip8->ram[0], font, sizeof(font));

    // Open ROM
    FILE *rom = fopen(rom_name, "rb");
    if (!rom)
    {
        SDL_Log("Rom file %s is invalid or does not exist !\n", rom_name);
        return false;
    }

    // Get/check rom size
    fseek(rom, 0, SEEK_END);
    const size_t rom_size = ftell(rom);
    const size_t max_size = sizeof(chip8->ram) - entry_point;
    rewind(rom);

    if (rom_size > max_size)
    {
        SDL_Log("Rom file %s is too big ! Rom size %zu, Max size allowed: %zu\n", rom_name, rom_size, max_size);
        return false;
    }

    if (fread(&chip8->ram[entry_point], rom_size, 1, rom) != 1)
    {
        SDL_Log("Could not read Rom file %s into CHIP8 memory !\n", rom_name);
        return false;
    }

    fclose(rom);

    // Set Chip8
    chip8->state = RUNNING;            // Default state
    chip8->PC = (uint16_t)entry_point; // Start program at entry point
    chip8->rom_name = rom_name;
    chip8->stack_ptr = &chip8->stack[0];
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
        .background_color = 0x00000000, // Original color as black bg
        .scale_factor = 20,             // Default res will be 1280x640
        .pixel_outlines = true,         // Draw pixel outlines by default
    };

    // Override defaults
    for (int i = 1; i < argc; i++)
    {
        (void)argv[i];
    }

    return true;
}

// Clear screen / SDL Window to background color
void sdl_clear_screen(const sdl_t sdl, const config_t config)
{
    const uint8_t r = (config.background_color >> 24) & 0xFF;
    const uint8_t g = (config.background_color >> 16) & 0xFF;
    const uint8_t b = (config.background_color >> 8) & 0xFF;
    const uint8_t a = (config.background_color >> 0) & 0xFF;

    SDL_SetRenderDrawColor(sdl.renderer, r, g, b, a);
    SDL_RenderClear(sdl.renderer);
}

// Update window
void update_screen(const sdl_t sdl, const config_t config, const chip8_t chip8) {
    SDL_FRect rect = {
        .x = 0,
        .y = 0,
        .w = (float)config.scale_factor,
        .h = (float)config.scale_factor,
    };

    // Grap color values to draw
    const uint8_t fg_r = (config.foreground_color >> 24) & 0xFF;
    const uint8_t fg_g = (config.foreground_color >> 16) & 0xFF;
    const uint8_t fg_b = (config.foreground_color >> 8) & 0xFF;
    const uint8_t fg_a = (config.foreground_color >> 0) & 0xFF;

    const uint8_t bg_r = (config.background_color >> 24) & 0xFF;
    const uint8_t bg_g = (config.background_color >> 16) & 0xFF;
    const uint8_t bg_b = (config.background_color >> 8) & 0xFF;
    const uint8_t bg_a = (config.background_color >> 0) & 0xFF;

    // Loop through display pixels, draw a rectangle per pixel to the SDL window
    for(uint32_t i = 0; i < sizeof(chip8.display); i++) {
        // Translate 1D index i value to 2D X/Y coordinates
        // X = i % window width
        // Y = i / window width
        rect.x = (float)((i % config.window_width) * config.scale_factor);
        rect.y = (float)((i / config.window_width) * config.scale_factor);

        if(chip8.display[i]) {
            // If pixel is on, draw foreground color
            SDL_SetRenderDrawColor(sdl.renderer, fg_r, fg_g, fg_b, fg_a);
            SDL_RenderFillRect(sdl.renderer, &rect);

            // if user requested drawing pixel outlines
            if(config.pixel_outlines) {
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
            case SDLK_SPACE:
                if (chip8->state == RUNNING)
                {
                    chip8->state = PAUSED; // Pause
                    puts("======= PAUSED =======");
                }
                else
                    chip8->state = RUNNING; // Resume
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

#ifdef DEBUG
void print_debug_info(chip8_t *chip8)
{
    printf("Address: 0x%04X, Opcode: 0x%04X Description: ", chip8->PC - 2, chip8->inst.opcode);
    
    switch ((chip8->inst.opcode >> 12) & 0x0F)
    {
        case 0x0:
            if (chip8->inst.NN == 0xE0)
            {
                // 0x00E0: Clear the screen
                printf("Clear screen\n");
            }
            else if (chip8->inst.NN == 0xEE)
            {
                // 0x00EE: return from subroutine
                // Set program counter to last address on subroutine stack (pop)
                // so that the next opcode value is used from there
                printf("Return from subroutine to address 0x%04X\n", *(chip8->stack_ptr - 1));
            }
            break;
        case 0x01:
            // 0x1NNN: Jumps to address NNN
            printf("Jump to address NNN (0x%04X)\n", chip8->inst.NNN);
            break;
        case 0x02:
            // 0x2NNN: Call subroutine at NNN
            // Store current address return to subroutine stack (push)
            // Set program counter to subroutine address so that the next opcode value is used from there
            printf("Call subroutine at NNN (0x%04X)\n", chip8->inst.NNN);
            break;
        case 0x03:
            // 0x3XNN: Skip next instruction if VX equals NN
            printf("Check if V%X (0x%02X) == NN (0x%02X), skip next instruction if true\n", chip8->inst.X,
                                                                                        chip8->V[chip8->inst.X], chip8->inst.NN);
            break;
        case 0x04:
            // 0x4XNN: Skip next instruction if VX is not equal to NN
            printf("Check if V%X (0x%02X) != NN (0x%02X), skip next instruction if true\n", chip8->inst.X,
                                                                                        chip8->V[chip8->inst.X], chip8->inst.NN);
            break;
        case 0x05: 
            if(chip8->inst.N != 0)
                break;
            // 0x5XY0: Check if VX equals VY if so, skip the next instruction
            printf("Check if V%X (0x%02X) == V%X (0x%02X), skip next instruction if true\n", chip8->inst.X, chip8->V[chip8->inst.X],
                                                                                             chip8->inst.Y, chip8->V[chip8->inst.Y]);
            break;
        case 0x06: 
            // 0x6XNN: Set register VX to NN
            printf("Set register V%X = NN (0x%02X)\n", chip8->inst.X, chip8->inst.NN);
            break;
        case 0x07:
            // 0x7XNN: Set register VX += NN
            printf("Set register V%X (0x%02X) += NN (0x%02X). Result: 0x%02X\n", chip8->inst.X, 
                                                                chip8->V[chip8->inst.X], chip8->inst.NN, 
                                                                chip8->V[chip8->inst.X] + chip8->inst.NN);
            break;
    case 0x08:
        switch(chip8->inst.N) {
            case 0:
                // 0x8XY0: Set VX to the value of VY
                printf("Set register V%X = V%X (0x%02X)\n", chip8->inst.X, 
                                                            chip8->inst.Y, chip8->V[chip8->inst.Y]);
                break;
            case 1:
                // 0x8XY1: Sets VX to VX or VY. (bitwise OR operation)
                printf("Set register V%X (0x%02X) |= V%X (0x%02X); Result: 0x%02X\n", 
                                                            chip8->inst.X, chip8->V[chip8->inst.X],
                                                            chip8->inst.Y, chip8->V[chip8->inst.Y],
                                                            chip8->V[chip8->inst.X] | chip8->V[chip8->inst.Y]);
                break;
            case 2:
                // 0x8XY2: Sets VX to VX and VY. (bitwise AND operation)
                printf("Set register V%X (0x%02X) &= V%X (0x%02X); Result: 0x%02X\n", 
                                                            chip8->inst.X, chip8->V[chip8->inst.X] ,
                                                            chip8->inst.Y, chip8->V[chip8->inst.Y],
                                                            chip8->V[chip8->inst.X] & chip8->V[chip8->inst.Y]);
                break;
            case 3:
                // 0x08Y3: Sets VX to VX xor VY.
                printf("Set register V%X (0x%02X) ^= V%X (0x%02X); Result: 0x%02X\n", 
                                                            chip8->inst.X, chip8->V[chip8->inst.X], 
                                                            chip8->inst.Y, chip8->V[chip8->inst.Y],
                                                            chip8->V[chip8->inst.X] ^ chip8->V[chip8->inst.Y]);
                break;
            case 4:
                // 0x08Y4: Adds VY to VX. VF is set to 1 when there's an overflow, and to 0 when there is not.
                printf("Set register V%X (0x%02X) += V%X (0x%02X); VF = 1 if carry; Result: 0x%02X VF=%X\n", 
                                                            chip8->inst.X, chip8->V[chip8->inst.X], 
                                                            chip8->inst.Y, chip8->V[chip8->inst.Y],
                                                            chip8->V[chip8->inst.X] + chip8->V[chip8->inst.Y],
                                                            (uint16_t)(chip8->V[chip8->inst.X] + chip8->V[chip8->inst.Y]) > 255);
            case 5:
                // 0x08Y5: VY is subtracted from VX. VF is set to 0 when there's an underflow, and 1 when there is not. (i.e. VF set to 1 if VX >= VY and 0 if not)
                printf("Set register V%X (0x%02X) -= V%X (0x%02X); VF = 1 if no borrow; Result: 0x%02X VF=%X\n", 
                                                            chip8->inst.X, chip8->V[chip8->inst.X], 
                                                            chip8->inst.Y, chip8->V[chip8->inst.Y],
                                                            chip8->V[chip8->inst.X] + chip8->V[chip8->inst.Y],
                                                            chip8->V[chip8->inst.Y] <= chip8->V[chip8->inst.X]);
                break;
            case 6:
                // 0x08Y6: Shifts VX to the right by 1, then stores the least significant bit of VX prior to the shift into VF.
                printf("Set register V%X (0x%02X) >>=1  VF = shifted off bit (%X); Result: 0x%02X \n", 
                                                            chip8->inst.X, chip8->V[chip8->inst.X], 
                                                            chip8->V[chip8->inst.X] & 1,
                                                            chip8->V[chip8->inst.X] >> 1);
                break;
            case 7:
                // 0x08Y7: VX is subtracted from VY. VF is set to 0 when there's an underflow, and 1 when there is not. (i.e. VF set to 1 if VY >= VX and 0 if not)
                printf("Set register V%X (0x%02X) -= V%X (0x%02X); VF = 1 if no borrow; Result: 0x%02X VF=%X\n", 
                                                            chip8->inst.Y, chip8->V[chip8->inst.Y], 
                                                            chip8->inst.X, chip8->V[chip8->inst.X],
                                                            chip8->V[chip8->inst.Y] + chip8->V[chip8->inst.X],
                                                            chip8->V[chip8->inst.X] <= chip8->V[chip8->inst.Y]);
                break;
            case 0XE:
                // 0x08XYE: Shifts VX to the left by 1, then sets VF to 1 if the most significant bit of VX prior to that shift was set, or to 0 if it was unset.
                printf("Set register V%X (0x%02X) <<=1  VF = shifted off bit (%X); Result: 0x%02X \n", 
                                                            chip8->inst.X, chip8->V[chip8->inst.X], 
                                                            chip8->V[chip8->inst.X] & 80 >> 7,
                                                            chip8->V[chip8->inst.X] << 1);
                break;
            default:
                break; // Unimplemented or invalid opcode
        }
        case 0x0A:
            // 0xANNN: Set index register I to NNN
            printf("Set I to NNN (0x%04X)\n", chip8->inst.NNN);
            break;
        case 0x0D:
        // 0x0DXYN: Draw N height sprite at coordinates X, Y; Read from memory location I
        // Screen pixels are XOR'd with sprite bits
        // VF (Carry flag) is set if any screen pixels are set off; This is useful
        // for collision detection or other reasons.
            printf("Draw N (%d) height sprite at coords V%X (0x%02X), V%X, (0x%02X)"
                 "from memory location I (0x%04X). Set VF = 1 if any pixels are turned off.\n", 
                chip8->inst.N, chip8->inst.X, chip8->V[chip8->inst.X], chip8->inst.Y, 
                chip8->V[chip8->inst.Y], chip8->I);
            break;
        default:
            printf("Unimplemented or invalid opcode !\n");
            break; // Unimplemented or invalid opcode
    }
}
#endif

// Emulate 1 instruction
void emulate_instruction(chip8_t *chip8, const config_t config)
{
    // Get next opcode from ram
    chip8->inst.opcode = (chip8->ram[chip8->PC] << 8) | chip8->ram[chip8->PC + 1];
    chip8->PC += 2; // Pre increment program counter for next opcode

    // Fill out current instruction format
    // DXYN
    chip8->inst.NNN = chip8->inst.opcode & 0xFFF;
    chip8->inst.NN = chip8->inst.opcode & 0x0FF;
    chip8->inst.N = chip8->inst.opcode & 0x0F;
    chip8->inst.X = (chip8->inst.opcode >> 8) & 0x0F;
    chip8->inst.Y = (chip8->inst.opcode >> 4) & 0x0FF;

#ifdef DEBUG
    print_debug_info(chip8);
#endif

    // Emulate opcode
    switch ((chip8->inst.opcode >> 12) & 0x0F)
    {
    case 0x0:
        if (chip8->inst.NN == 0xE0)
        {
            // 0x00E0: Clear the screen
            memset(&chip8->display[0], false, sizeof(chip8->display));
        }
        else if (chip8->inst.NN == 0xEE)
        {
            // 0x00EE: return from subroutine
            // Set program counter to last address on subroutine stack (pop)
            // so that the next opcode value is used from there
            chip8->PC = *--chip8->stack_ptr;
        } else {
            // Unimplemented / invalid opcode maybe NNN
        }
        break;
    case 0x01:
        // 0x1NNN: Jumps to address NNN
        chip8->PC = chip8->inst.NNN; // Set program counter so that next opcode is NNN
        break;
    case 0x02:
        // 0x2NNN: Call subroutine at NNN
        // Store current address return to subroutine stack (push)
        // Set program counter to subroutine address so that the next opcode value is used from there
        *chip8->stack_ptr++ = chip8->PC;
        chip8->PC = chip8->inst.NNN;
        break;
    case 0x03:
        // 0x3XNN: Skip next instruction if VX equals NN
        if(chip8->V[chip8->inst.X] == chip8->inst.NN)
            chip8->PC += 2; // Skip next opcode/instruction
        break;
    case 0x04:
        // 0x4XNN: Skip next instruction if VX is not equal to NN
        if(chip8->V[chip8->inst.X] != chip8->inst.NN)
            chip8->PC += 2; // Skip next opcode/instruction
        break;
    case 0x05: 
        if(chip8->inst.N != 0)
            break;
        // 0x5XY0: Check if VX equals VY if so, skip the next instruction
        if(chip8->V[chip8->inst.X] == chip8->V[chip8->inst.Y])
            chip8->PC += 2; // Skip next opcode/instruction
        break;
    case 0x06: 
        // 0x6XNN: Set register VX to NN
        chip8->V[chip8->inst.X] = chip8->inst.NN;
        break;
    case 0x07:
        // 0x7XNN: Set register VX += NN
        chip8->V[chip8->inst.X] += chip8->inst.NN;
        break;
    case 0x08:
        switch(chip8->inst.N) {
            case 0:
                // 0x8XY0: Set VX to the value of VY
                chip8->V[chip8->inst.X] = chip8->V[chip8->inst.Y];
                break;
            case 1:
                // 0x8XY1: Sets VX to VX or VY. (bitwise OR operation)
                chip8->V[chip8->inst.X] |= chip8->V[chip8->inst.Y];
                break;
            case 2:
                // 0x8XY2: Sets VX to VX and VY. (bitwise AND operation)
                chip8->V[chip8->inst.X] &= chip8->V[chip8->inst.Y];
                break;
            case 3:
                // 0x08Y3: Sets VX to VX xor VY.
                chip8->V[chip8->inst.X] ^= chip8->V[chip8->inst.Y];
                break;
            case 4:
                // 0x08Y4: Adds VY to VX. VF is set to 1 when there's an overflow, and to 0 when there is not.
                if((uint16_t)(chip8->V[chip8->inst.X] + chip8->V[chip8->inst.Y]) > 255)
                    chip8->V[0xF] = 1;

                chip8->V[chip8->inst.X] += chip8->V[chip8->inst.Y];
                break;
            case 5:
                // 0x08Y5: VY is subtracted from VX. VF is set to 0 when there's an underflow, and 1 when there is not. (i.e. VF set to 1 if VX >= VY and 0 if not)
                if(chip8->V[chip8->inst.Y] <= chip8->V[chip8->inst.X])
                    chip8->V[0xF] = 1;

                chip8->V[chip8->inst.X] -= chip8->V[chip8->inst.Y];
                break;
            case 6:
                // 0x08Y6: Shifts VX to the right by 1, then stores the least significant bit of VX prior to the shift into VF.
                chip8->V[0xF] = chip8->V[chip8->inst.X] & 1;
                chip8->V[chip8->inst.X] >>= 1;
                break;
            case 7:
                // 0x08Y7: VX is subtracted from VY. VF is set to 0 when there's an underflow, and 1 when there is not. (i.e. VF set to 1 if VY >= VX and 0 if not)
                if(chip8->V[chip8->inst.X] <= chip8->V[chip8->inst.Y])
                    chip8->V[0xF] = 1;

                chip8->V[chip8->inst.Y] -= chip8->V[chip8->inst.X];
                break;
            case 0XE:
                // 0x08XYE: Shifts VX to the left by 1, then sets VF to 1 if the most significant bit of VX prior to that shift was set, or to 0 if it was unset.
                chip8->V[0xF] = (chip8->V[chip8->inst.X] & 0x80) >> 7;
                chip8->V[chip8->inst.X] <<= 1;
                break;
            default:
                break; // Unimplemented or invalid opcode
        }

    case 0x0A:
        // 0xANNN: Set index register I to NNN
        chip8->I = chip8->inst.NNN;
        break;
    case 0x0D:
        // 0x0DXYN: Draw N height sprite at coordinates X, Y; Read from memory location I
        // Screen pixels are XOR'd with sprite bits
        // VF (Carry flag) is set if any screen pixels are set off; This is useful
        // for collision detection or other reasons.
        uint8_t X_coord = chip8->V[chip8->inst.X] % config.window_width;
        uint8_t Y_coord = chip8->V[chip8->inst.Y] % config.window_height;
        const uint8_t orig_X = X_coord; // Original X value

        chip8->V[0xF] = 0; // Initialize carry flag to 0

        for(uint8_t i = 0; i < chip8->inst.N; i++) {
            // Get next byte/row of sprite data
            const uint8_t sprite_data = chip8->ram[chip8->I + i];
            X_coord = orig_X; // Reset x for next row to draw 
            
            for(int8_t j = 7; j >= 0; j--) {
                // If sprite pixel/bit is on and display pixel is on, set carry flag
                bool *pixel =  &chip8->display[Y_coord * config.window_width + X_coord];
                const bool sprite_bit = (sprite_data & (1 << j));

                if(sprite_bit && *pixel) {
                    chip8->V[0xF] = 1; // Set carry flag to 1
                }

                // XOR display pixel width sprite pixel/bit
                *pixel ^= sprite_bit;

                // Stop drawing this row if hit right edge of screen
                if(++X_coord >= config.window_width)
                    break;
            }
            // Stop drawing entire sprite if hit bottom edge of screen
            if(++Y_coord >= config.window_height) 
                break;
        }
        break;
        
    default:
        break; // Unimplemented or invalid opcode
    }
}

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
    if (!init_sdl(&sdl, config))
        exit(EXIT_FAILURE);

    chip8_t chip8 = {0};
    const char *rom_name = argv[1];
    if (!init_chip8(&chip8, rom_name))
        exit(EXIT_FAILURE);

    // Main emulator loop
    while (chip8.state != QUIT)
    {
        // Handle user input
        handle_input(&chip8);

        // Initial screen clear to background color
        //clear_screen(sdl, config);

        if(chip8.state == PAUSED) continue;

        // get_time();

        // Emulate Chip-8 instructions
        emulate_instruction(&chip8, config);

        // get_time() elapsed since last get_time();

        // Delay for approximately 60hz/60fps
        // SDL_Delay(16 - time elapsed);
        SDL_Delay(16);

        // Update
        update_screen(sdl, config, chip8);
    }

    // Final cleanup
    final_cleanup(sdl);

    exit(EXIT_SUCCESS);
}