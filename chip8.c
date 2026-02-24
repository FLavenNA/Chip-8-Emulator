#include "chip8.h"
#include "app.h"
#include "sdl.h"
#include "instruction_tables.h"

bool init_chip8(chip8_t *chip8, const config_t *config, const char rom_name[])
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

    // Initialize entire CHIP8 machine
    memset(chip8, 0, sizeof(chip8_t));

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
    memset(&chip8->pixel_color[0], config->background_color, sizeof(chip8->pixel_color)); // Init pixels to background color

    return true;
}

void handle_input(chip8_t *chip8, config_t *config)
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
                break;
            case SDLK_SPACE:
                if (chip8->state == RUNNING)
                {
                    chip8->state = PAUSED; // Pause
                    puts("======= PAUSED =======");
                }
                else
                    chip8->state = RUNNING; // Resume
                break;
            case SDLK_ASTERISK:
                // '*': Reset CHIP8 machine for current rom
                init_chip8(chip8, config, chip8->rom_name);
                break;
            case SDLK_J:
                // 'J': Decrease color lerp rate
                if(config->color_lerp_rate > 0.1)
                    config->color_lerp_rate -= 0.1f;
                break;
            case SDLK_K:
                // 'K': Increase color lerp rate
                if(config->color_lerp_rate < 0.9)
                    config->color_lerp_rate += 0.1f;
                break;
            case SDLK_O:
                // 'O': Decrease volume
                if(config->volume > 0)
                    config->volume -= 500;
                break;
            case SDLK_P:
                // 'P': Increase volume
                if(config->volume < INT16_MAX)
                    config->volume += 500;
                break;
            default:
                break;
            }
            for (int i = 0; i < NUM_KEYS; i++) {
                if (event.key.key == KEYMAP[i][0]) {
                    chip8->keypad[KEYMAP[i][1]] = true;
                }
            }
            break;

        case SDL_EVENT_KEY_UP:
            for (int i = 0; i < NUM_KEYS; i++) {
                if (event.key.key == KEYMAP[i][0]) {
                    chip8->keypad[KEYMAP[i][1]] = false;
                }
            }
            break;
        default:
            break;
        }
    }
}

// TODO: Not sure this is the right way or it works correctly check later
void handle_audio(chip8_t *chip8, const config_t *config, SDL_AudioStream *stream)
{
    // Calculate number of samples to generate based on your audio format
    int num_samples = config->audio_sample_rate / 75; // generate ~20ms of audio per callback
    int16_t *temp_buffer = malloc(num_samples * sizeof(int16_t));
    if (!temp_buffer) return;

    static uint32_t running_sample_index = 0;
    const int32_t square_wave_period = config->audio_sample_rate / config->square_wave_freq;
    const int32_t half_square_wave_period = square_wave_period / 2;

    if (chip8->sound_timer == 0) {
        memset(temp_buffer, 0, num_samples * sizeof(int16_t));
    } else {
        for (int i = 0; i < num_samples; i++) {
            temp_buffer[i] = ((running_sample_index++ / half_square_wave_period) % 2)
                                ?  config->volume
                                : -config->volume;
        }
    }
    // Push the generated samples into the SDL_AudioStream
    SDL_PutAudioStreamData(stream, temp_buffer, num_samples * sizeof(int16_t));

    free(temp_buffer);
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
                // 0x8XY3: Sets VX to VX xor VY.
                printf("Set register V%X (0x%02X) ^= V%X (0x%02X); Result: 0x%02X\n", 
                                                            chip8->inst.X, chip8->V[chip8->inst.X], 
                                                            chip8->inst.Y, chip8->V[chip8->inst.Y],
                                                            chip8->V[chip8->inst.X] ^ chip8->V[chip8->inst.Y]);
                break;
            case 4:
                // 0x8XY4: Adds VY to VX. VF is set to 1 when there's an overflow, and to 0 when there is not.
                printf("Set register V%X (0x%02X) += V%X (0x%02X); VF = 1 if carry; Result: 0x%02X VF=%X\n", 
                                                            chip8->inst.X, chip8->V[chip8->inst.X], 
                                                            chip8->inst.Y, chip8->V[chip8->inst.Y],
                                                            chip8->V[chip8->inst.X] + chip8->V[chip8->inst.Y],
                                                            (uint16_t)(chip8->V[chip8->inst.X] + chip8->V[chip8->inst.Y]) > 255);
            case 5:
                // 0x8XY5: VY is subtracted from VX. VF is set to 0 when there's an underflow, and 1 when there is not. (i.e. VF set to 1 if VX >= VY and 0 if not)
                printf("Set register V%X (0x%02X) -= V%X (0x%02X); VF = 1 if no borrow; Result: 0x%02X VF=%X\n", 
                                                            chip8->inst.X, chip8->V[chip8->inst.X], 
                                                            chip8->inst.Y, chip8->V[chip8->inst.Y],
                                                            chip8->V[chip8->inst.X] + chip8->V[chip8->inst.Y],
                                                            chip8->V[chip8->inst.Y] <= chip8->V[chip8->inst.X]);
                break;
            case 6:
                // 0x8XY6: Shifts VX to the right by 1, then stores the least significant bit of VX prior to the shift into VF.
                printf("Set register V%X (0x%02X) >>=1  VF = shifted off bit (%X); Result: 0x%02X \n", 
                                                            chip8->inst.X, chip8->V[chip8->inst.X], 
                                                            chip8->V[chip8->inst.X] & 1,
                                                            chip8->V[chip8->inst.X] >> 1);
                break;
            case 7:
                // 0x8XY7: VX is subtracted from VY. VF is set to 0 when there's an underflow, and 1 when there is not. (i.e. VF set to 1 if VY >= VX and 0 if not)
                printf("Set register V%X (0x%02X) -= V%X (0x%02X); VF = 1 if no borrow; Result: 0x%02X VF=%X\n", 
                                                            chip8->inst.Y, chip8->V[chip8->inst.Y], 
                                                            chip8->inst.X, chip8->V[chip8->inst.X],
                                                            chip8->V[chip8->inst.Y] + chip8->V[chip8->inst.X],
                                                            chip8->V[chip8->inst.X] <= chip8->V[chip8->inst.Y]);
                break;
            case 0XE:
                // 0x8XYE: Shifts VX to the left by 1, then sets VF to 1 if the most significant bit of VX prior to that shift was set, or to 0 if it was unset.
                printf("Set register V%X (0x%02X) <<=1  VF = shifted off bit (%X); Result: 0x%02X \n", 
                                                            chip8->inst.X, chip8->V[chip8->inst.X], 
                                                            chip8->V[chip8->inst.X] & 80 >> 7,
                                                            chip8->V[chip8->inst.X] << 1);
                break;
            default:
                break; // Unimplemented or invalid opcode
        }
        case 0x09:
            // 0x9XY0: Skips the next instruction if VX does not equal VY. (Usually the next instruction is a jump to skip a code block).
            printf("Check if V%X (0x%02X) != V%X (0x%02X), skip next instruction if true\n", chip8->inst.X, chip8->V[chip8->inst.X],
                                                                                             chip8->inst.Y, chip8->V[chip8->inst.Y]);
            break;
        case 0x0A:
            // 0xANNN: Set index register I to NNN
            printf("Set I to NNN (0x%04X)\n", chip8->inst.NNN);
            break;
        case 0x0B:
            // 0xBNNN: Jumps to the address NNN plus V0
            printf("Set PC to V0 (0x%02X) + NNN (0x%04x); Result PC = 0x%04X\n", chip8->V[0], chip8->inst.NNN,
                                                             chip8->inst.NNN + chip8->V[0]);
            break;
        case 0x0C:
            // 0xCNNN: Set Vx = rand() & NN. rand() creates a number between 0-255 
            printf("Set V%X = rand() %% 256 & NN (0x%02X)\n", chip8->inst.X, chip8->inst.NN);
            break;
        case 0x0D:
        // 0x0DXYN: Draw N height sprite at coordinates X, Y; Read from memory location I
        // Screen pixels are XOR'd with sprite bits
        // VF (Carry flag) is set if any screen pixels are set off; This is useful
        // for collision detection or other reasons.
            printf("Draw N (%d) height sprite at coords V%X (0x%02X), V%X (0x%02X)"
                 "from memory location I (0x%04X). Set VF = 1 if any pixels are turned off.\n", 
                chip8->inst.N, chip8->inst.X, chip8->V[chip8->inst.X], chip8->inst.Y, 
                chip8->V[chip8->inst.Y], chip8->I);
            break;
        case 0x0E: 
            if(chip8->inst.NN == 0x9E) {
                // 0xEX9E: Skip next instruction if key in VX is pressed
                printf("Skip next instruction if key in V%X (0x%02X) is pressed, Keypad value %d\n",
                                                                                                chip8->inst.X, chip8->V[chip8->inst.X],
                                                                                                chip8->keypad[chip8->V[chip8->inst.X]]);
            } else if(chip8->inst.NN == 0xA1) {
                // 0xEX9E: Skip next instruction if key in VX is not pressed
                printf("Skip next instruction if key in V%X (0x%02X) is not pressed, Keypad value %d\n",
                                                                                                chip8->inst.X, chip8->V[chip8->inst.X],
                                                                                                chip8->keypad[chip8->V[chip8->inst.X]]);
            }
            break;
        case 0x0F:
            switch (chip8->inst.NN) {
            case 0x0A:
                // 0xFX0A: A key press is awaited, and then stored in VX (blocking operation, all instruction halted until next key event, delay and sound timers should continue processing).
                printf("Await until a key is pressed; Store key in V%X\n", chip8->inst.X);
                break;
            case 0x1E:
                // 0xFX1E: Adds VX to I. VF is not affected. For non-Amiga CHIP8, does not affect VF
                printf("I (0x%04X) += V%X (0x%02X); Result (I): 0x%04X\n", chip8->I, chip8->inst.X, 
                                                                           chip8->V[chip8->inst.X], chip8->I + chip8->V[chip8->inst.X]);
                break;
            case 0x07:
                // 0xFX07: VX = delay timer
                printf("Set V%X = delay timer value (0x%02X)\n", chip8->inst.X, chip8->delay_timer);
                break;
            case 0x15:
                // 0xFX15: delay timer = VX
                printf("Set delay timer value = V%X (0x%02X)\n", chip8->inst.X, chip8->V[chip8->inst.X]);
                break;
            case 0x18:
                // 0xFX18: VX = sonud timer
                printf("Set V%X = sound timer value (0x%02X)\n", chip8->inst.X, chip8->sound_timer);
                break;
            case 0x29:
                // 0xF29: Set register I to sprite location in memory for character in VX (0x0 - 0xF)
                printf("Set I to sprite location in memory for character in V%X (0x%02X). Result(VX*5) = (0x%02X)\n", 
                                                                                            chip8->inst.X, chip8->V[chip8->inst.X],
                                                                                            chip8->V[chip8->inst.X] * 5);
                break;
            case 0x33:
                // 0xFX33: Store BCD representation of VX at memory offset from I
                // I = hundred's place, I+1 = ten's place, I+2 = one's place;
                printf("Store BCD representation of V%X (0x%02X) at memory from I (0x%04X)\n", chip8->inst.X,
                                                                                            chip8->V[chip8->inst.X], chip8->I);
                break;
            case 0x55:
                // 0xFX55: Register dump V0-VX inclusive to memory offset from I;
                // SCHIP does not increment I, CHIP8 does increment I;
                printf("Register dump  V0-V%X (0x%02X) inclusive at memory from I (0x%04X)\n", chip8->inst.X,
                                                                                            chip8->V[chip8->inst.X], chip8->I);
                break;
            case 0x65:
                // 0xFX65: Register load V0-VX inclusive to memory offset from I;
                // SCHIP does not increment I, CHIP8 does increment I;
                printf("Register load  V0-V%X (0x%02X) inclusive at memory from I (0x%04X)\n", chip8->inst.X,
                                                                                            chip8->V[chip8->inst.X], chip8->I);
                break;
            default:
                break;
            }
            break;
        default:
            printf("Unimplemented or invalid opcode !\n");
            break; // Unimplemented or invalid opcode
    }
}
#endif

// Emulate 1 instruction
void emulate_instruction(chip8_t *chip8, const config_t *config)
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
    chip8->inst.Y = (chip8->inst.opcode >> 4) & 0x0F;

#ifdef DEBUG
    print_debug_info(chip8);
#endif

    // Emulate opcode
    // Dispatch using high nibble
    uint8_t high_nibble = (chip8->inst.opcode >> 12) & 0x0F;
    opcode_table[high_nibble](chip8, config);
}

void update_timers(const sdl_t *sdl, chip8_t *chip8) {
    if(chip8->delay_timer > 0) chip8->delay_timer--;
    if(chip8->sound_timer > 0) {
        chip8->sound_timer--;
        SDL_PauseAudioStreamDevice(sdl->stream);
    }
    else {
        SDL_ResumeAudioStreamDevice(sdl->stream);
    }
}

void instr_0NNN(chip8_t *chip8, const config_t *config) {
    (void)config;

    if (table_0NNN[chip8->inst.NN])
        table_0NNN[chip8->inst.NN](chip8, config);
    else{
        // Unimplemented / invalid opcode maybe NNN
    }
}

void instr_00E0(chip8_t *chip8, const config_t *config) {
    (void)config;

    // 0x00E0: Clear the screen
    memset(&chip8->display[0], false, sizeof(chip8->display));
    chip8->draw = true; // Will update screen on next 60hz tick
}

void instr_00EE(chip8_t *chip8, const config_t *config) {
    (void)config;

    // 0x00EE: return from subroutine
    // Set program counter to last address on subroutine stack (pop)
    // so that the next opcode value is used from there
    chip8->PC = *--chip8->stack_ptr;
}

void instr_1NNN(chip8_t *chip8, const config_t *config) {
    (void)config;

    // 0x1NNN: Jumps to address NNN
    chip8->PC = chip8->inst.NNN; // Set program counter so that next opcode is NNN
}

void instr_2NNN(chip8_t *chip8, const config_t *config) {
    (void)config;

    // 0x2NNN: Call subroutine at NNN
    // Store current address return to subroutine stack (push)
    // Set program counter to subroutine address so that the next opcode value is used from there
    *chip8->stack_ptr++ = chip8->PC;
    chip8->PC = chip8->inst.NNN;
}

void instr_3XNN(chip8_t *chip8, const config_t *config) {
    (void)config;

    // 0x3XNN: Skip next instruction if VX equals NN
    if (chip8->V[chip8->inst.X] == chip8->inst.NN)
        chip8->PC += 2; // Skip next opcode/instruction
}

void instr_4XNN(chip8_t *chip8, const config_t *config) {
    (void)config;

    // 0x4XNN: Skip next instruction if VX is not equal to NN
    if (chip8->V[chip8->inst.X] != chip8->inst.NN)
        chip8->PC += 2; // Skip next opcode/instruction
}

void instr_5XY0(chip8_t *chip8, const config_t *config) {
    (void)config;

    if (chip8->inst.N != 0)
        return;
    // 0x5XY0: Check if VX equals VY if so, skip the next instruction
    if (chip8->V[chip8->inst.X] == chip8->V[chip8->inst.Y])
        chip8->PC += 2; // Skip next opcode/instruction
}

void instr_6XNN(chip8_t *chip8, const config_t *config) {
    (void)config;

    // 0x6XNN: Set register VX to NN
    chip8->V[chip8->inst.X] = chip8->inst.NN;
}

void instr_7XNN(chip8_t *chip8, const config_t *config) {
    (void)config;

    // 0x7XNN: Set register VX += NN
    chip8->V[chip8->inst.X] += chip8->inst.NN;
}

void instr_8XYN(chip8_t *chip8, const config_t *config) {
    (void)config;

    if(table_8XYN[chip8->inst.N])
        table_8XYN[chip8->inst.N](chip8, config);
}

void instr_8XY0(chip8_t *chip8, const config_t *config) {
    (void)config;

    // 0x8XY0: Set VX to the value of VY
    chip8->V[chip8->inst.X] = chip8->V[chip8->inst.Y];
}

void instr_8XY1(chip8_t *chip8, const config_t *config) {
    (void)config;

    // 0x8XY1: Sets VX to VX or VY. (bitwise OR operation)
    chip8->V[chip8->inst.X] |= chip8->V[chip8->inst.Y];
    if(config->current_extension == CHIP8)
        chip8->V[0xF] = 0; // Chip-8 ONLY QUIRK
}

void instr_8XY2(chip8_t *chip8, const config_t *config) {
    (void)config;

    // 0x8XY2: Sets VX to VX and VY. (bitwise AND operation)
    chip8->V[chip8->inst.X] &= chip8->V[chip8->inst.Y];
    if(config->current_extension == CHIP8)
        chip8->V[0xF] = 0; // Chip-8 ONLY QUIRK
}

void instr_8XY3(chip8_t *chip8, const config_t *config) {
    (void)config;

    // 0x8XY3: Sets VX to VX xor VY.
    chip8->V[chip8->inst.X] ^= chip8->V[chip8->inst.Y];
    if(config->current_extension == CHIP8)
        chip8->V[0xF] = 0; // Chip-8 ONLY QUIRK
}

void instr_8XY4(chip8_t *chip8, const config_t *config) {
    (void)config;

    // 0x8XY4: Adds VY to VX. VF is set to 1 when there's an overflow, and to 0 when there is not.
    const bool carry = ((uint16_t)(chip8->V[chip8->inst.X] + chip8->V[chip8->inst.Y]) > 255);
    
    chip8->V[chip8->inst.X] += chip8->V[chip8->inst.Y];
    chip8->V[0xF] = carry;
}

void instr_8XY5(chip8_t *chip8, const config_t *config) {
    (void)config;

    // 0x8XY5: VY is subtracted from VX. VF is set to 0 when there's an underflow, and 1 when there is not. (i.e. VF set to 1 if VX >= VY and 0 if not)
    const bool carry = (chip8->V[chip8->inst.Y] <= chip8->V[chip8->inst.X]);
    
    chip8->V[chip8->inst.X] -= chip8->V[chip8->inst.Y];
    chip8->V[0xF] = carry;
}

void instr_8XY6(chip8_t *chip8, const config_t *config) {
    (void)config;

    // 0x8XY6: Shifts VX to the right by 1, then stores the least significant bit of VX prior to the shift into VF.
    bool carry;
    if(config->current_extension == CHIP8) {
        carry = chip8->V[chip8->inst.Y] & 1; // USE VY
        chip8->V[chip8->inst.X] = chip8->V[chip8->inst.Y] >> 1;
    }
    else
    {
        carry = chip8->V[chip8->inst.X] & 1; // USE VX
        chip8->V[chip8->inst.X] >>= 1;       // USE VX
    }
    
    chip8->V[0xF] = carry;
}

void instr_8XY7(chip8_t *chip8, const config_t *config) {
    (void)config;

    // 0x8XY7: VX is subtracted from VY. VF is set to 0 when there's an underflow, and 1 when there is not. (i.e. VF set to 1 if VY >= VX and 0 if not)
    const bool carry = (chip8->V[chip8->inst.X] <= chip8->V[chip8->inst.Y]);
    
    chip8->V[chip8->inst.Y] -= chip8->V[chip8->inst.X];
    chip8->V[0xF] = carry;
}

void instr_8XYE(chip8_t *chip8, const config_t *config) {
    (void)config;

    // 0x8XYE: Shifts VX to the left by 1, then sets VF to 1 if the most significant bit of VX prior to that shift was set, or to 0 if it was unset.
    bool carry;
    if(config->current_extension == CHIP8) {
        carry = (chip8->V[chip8->inst.Y] & 0x80) >> 7;          // Use VY
        chip8->V[chip8->inst.X] = chip8->V[chip8->inst.Y] << 1; // Set VX = VY;
    }
    else {
        carry = (chip8->V[chip8->inst.X] & 0x80) >> 7;  // VX
        chip8->V[chip8->inst.X] <<= 1;                  // Use VX
    }

    chip8->V[0xF] = carry;
}

void instr_9XY0(chip8_t *chip8, const config_t *config) {
    (void)config;

    // 0x9XY0: Skips the next instruction if VX does not equal VY. (Usually the next instruction is a jump to skip a code block).
    if (chip8->inst.N != 0)
        return;
    if (chip8->V[chip8->inst.X] != chip8->V[chip8->inst.Y])
        chip8->PC += 2;
}

void instr_ANNN(chip8_t *chip8, const config_t *config) {
    (void)config;

    // 0xANNN: Set index register I to NNN
    chip8->I = chip8->inst.NNN;
}

void instr_BNNN(chip8_t *chip8, const config_t *config) {
    (void)config;

    // 0xBNNN: Jumps to the address NNN plus V0
    chip8->PC = chip8->inst.NNN + chip8->V[0];
}

void instr_CXNN(chip8_t *chip8, const config_t *config) {
    (void)config;

    // 0xCNNN: Set Vx = rand() & NN. rand() creates a number between 0-255
    chip8->V[chip8->inst.X] = (rand() % 256) & chip8->inst.NN;
}

void instr_DXYN(chip8_t *chip8, const config_t *config) {
    // 0x0DXYN: Draw N height sprite at coordinates X, Y; Read from memory location I
    // Screen pixels are XOR'd with sprite bits
    // VF (Carry flag) is set if any screen pixels are set off; This is useful
    // for collision detection or other reasons.
    uint8_t X_coord = chip8->V[chip8->inst.X] % config->window_width;
    uint8_t Y_coord = chip8->V[chip8->inst.Y] % config->window_height;
    const uint8_t orig_X = X_coord; // Original X value

    chip8->V[0xF] = 0; // Initialize carry flag to 0

    for (uint8_t i = 0; i < chip8->inst.N; i++)
    {
        // Get next byte/row of sprite data
        const uint8_t sprite_data = chip8->ram[chip8->I + i];
        X_coord = orig_X; // Reset x for next row to draw

        for (int8_t j = 7; j >= 0; j--)
        {
            // If sprite pixel/bit is on and display pixel is on, set carry flag
            bool *pixel = &chip8->display[Y_coord * config->window_width + X_coord];
            const bool sprite_bit = (sprite_data & (1 << j));

            if (sprite_bit && *pixel)
            {
                chip8->V[0xF] = 1; // Set carry flag to 1
            }

            // XOR display pixel width sprite pixel/bit
            *pixel ^= sprite_bit;

            // Stop drawing this row if hit right edge of screen
            if (++X_coord >= config->window_width)
                break;
        }
        // Stop drawing entire sprite if hit bottom edge of screen
        if (++Y_coord >= config->window_height)
            break;
    }
    chip8->draw = true; // Will update screen on next 60hz tick
}

void instr_EXNN(chip8_t *chip8, const config_t *config){
    (void)config;

    if(table_EXNN[chip8->inst.NN])
        table_EXNN[chip8->inst.NN](chip8, config);
}

void instr_EX9E(chip8_t *chip8, const config_t *config) {
    (void)config;

    // 0xEX9E: Skip next instruction if key in VX is pressed
    if (chip8->keypad[chip8->V[chip8->inst.X]])
        chip8->PC += 2;
}

void instr_EXA1(chip8_t *chip8, const config_t *config) {
    (void)config;

    // 0xEX9E: Skip next instruction if key in VX is not pressed
    if (!chip8->keypad[chip8->V[chip8->inst.X]])
        chip8->PC += 2;
}

void instr_FXNN(chip8_t *chip8, const config_t *config) {
    (void)config;
    if(table_FXNN[chip8->inst.NN])
        table_FXNN[chip8->inst.NN](chip8, config);
}

void instr_FX07(chip8_t *chip8, const config_t *config) {
    (void)config;

    // 0xFX07: VX = delay timer
    chip8->V[chip8->inst.X] = chip8->delay_timer;
}

void instr_FX0A(chip8_t *chip8, const config_t *config) {
    (void)config;

    bool any_key_pressed = false;
    // 0xFX0A: A key press is awaited, and then stored in VX (blocking operation, all instruction halted until next key event, delay and sound timers should continue processing).
    for (uint8_t i = 0; i < sizeof(chip8->keypad); i++)
    {
        if (chip8->keypad[i])
        {
            chip8->V[chip8->inst.X] = i; // i = key (offset into keypad array)
            any_key_pressed = true;
            break;
        }
    }
    if (!any_key_pressed)
        chip8->PC -= 2; // Keep getting the current opcode if no key has been pressed
}

void instr_FX15(chip8_t *chip8, const config_t *config) {
    (void)config;

    // 0xFX15: delay timer = VX
    chip8->delay_timer = chip8->V[chip8->inst.X];
}

void instr_FX18(chip8_t *chip8, const config_t *config) {
    (void)config;
    // 0xFX18: VX = sound timer
    chip8->sound_timer = chip8->V[chip8->inst.X];
}

void instr_FX1E(chip8_t *chip8, const config_t *config) {
    (void)config;

    // 0xFX1E: Adds VX to I. VF is not affected. For non-Amiga CHIP8, does not affect VF
    chip8->I += chip8->V[chip8->inst.X];
}

void instr_FX29(chip8_t *chip8, const config_t *config) {
    (void)config;

    // 0xFX29: Set register I to sprite location in memory for character in VX (0x0 - 0xF)
    chip8->I = chip8->V[chip8->inst.X] * 5;
}

void instr_FX33(chip8_t *chip8, const config_t *config) {
    (void)config;

    // 0xFX33: Store BCD representation of VX at memory offset from I
    // I = hundred's place, I+1 = ten's place, I+2 = one's place;
    uint8_t bcd = chip8->V[chip8->inst.X];
    chip8->ram[chip8->I + 2] = bcd % 10;
    bcd /= 10;
    chip8->ram[chip8->I + 1] = bcd % 10;
    bcd /= 10;
    chip8->ram[chip8->I] = bcd;
}

void instr_FX55(chip8_t *chip8, const config_t *config) {
    (void)config;

    // 0xFX55: Register dump V0-VX inclusive to memory offset from I;
    // SCHIP does not increment I, CHIP8 does increment I;
    // Note: Could make this a config flag to use SCHIP or CHIP8 logic for I
    for (uint8_t i = 0; i <= chip8->inst.X; i++)
    {
        if(config->current_extension == CHIP8){
            chip8->ram[chip8->I++] = chip8->V[i];
        }
        else
            chip8->ram[chip8->I + i] = chip8->V[i];
    }
}

void instr_FX65(chip8_t *chip8, const config_t *config) {
    (void)config;

    // 0xFX65: Register load V0-VX inclusive to memory offset from I;
    // SCHIP does not increment I, CHIP8 does increment I;
    for (uint8_t i = 0; i <= chip8->inst.X; i++)
    {
        if(config->current_extension == CHIP8)
        {
            chip8->V[i] = chip8->ram[chip8->I++];
        }
        else
            chip8->V[i] = chip8->ram[chip8->I + i];
    }
}