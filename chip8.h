#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL3/SDL.h>

#include "type_defs.h"

#define NUM_KEYS 16

    // QWERTY           // CHIP8-KeyMap
const static uint8_t KEYMAP[NUM_KEYS][2] = {
    {SDLK_1, 0x1},      // 1 
    {SDLK_2, 0x2},      // 2
    {SDLK_3, 0x3},      // 3
    {SDLK_4, 0xC},      // C
    {SDLK_Q, 0x4},      // 4
    {SDLK_W, 0x5},      // 5
    {SDLK_E, 0x6},      // 6
    {SDLK_R, 0xD},      // D
    {SDLK_A, 0x7},      // 7
    {SDLK_S, 0x8},      // 8
    {SDLK_D, 0x9},      // 9
    {SDLK_F, 0xE},      // E
    {SDLK_Z, 0xA},      // A
    {SDLK_X, 0x0},      // 0
    {SDLK_C, 0xB},      // B
    {SDLK_V, 0xF}       // F
};

struct instruction
{
    uint16_t opcode;
    uint16_t NNN; // 12 bit address/constant
    uint8_t NN;   // 8 bit constant
    uint8_t N;    // 4 bit constant
    uint8_t X;    // 4 bit register identifier
    uint8_t Y;    // 4 bit register identifier
};

// Chip-8 machine object
struct chip8
{
    emulator_state_t state;
    uint8_t ram[4096];
    bool display[64 * 32]; // Emulator original resolution pixels
    uint16_t stack[12];    // Subroutine stack
    uint16_t *stack_ptr;
    uint8_t V[16];         // Data registers V0 - VF
    uint16_t I;            // Index register
    uint16_t PC;           // Program Counter
    uint8_t sound_timer;   // Decrement at 60hz and plays tone when > 0z
    uint8_t delay_timer;   // Decrement at 60hz when > 0
    bool keypad[16];       // Hexadecimal keypad 0x0 - 0xF
    const char *rom_name;  // Currently running ROM
    instruction_t inst;    // Currently executing instruction
    bool draw;             // Update the screen yes/no
};

typedef void (*instruction_func_t)(chip8_t *chip8, const config_t *config);

bool init_chip8(chip8_t *chip8, const char rom_name[]);
void handle_input(chip8_t *chip8);
void handle_audio(chip8_t *chip8, const config_t *config, SDL_AudioStream *stream);
#ifdef DEBUG
void print_debug_info(chip8_t *chip8);
#endif
void emulate_instruction(chip8_t *chip8, const config_t *config);
void update_timers(const sdl_t *sdl, chip8_t *chip8);

// Instructions 
// TODO: Was lazy to name them so made it like this change later maybe
void instr_00E0(chip8_t *chip8, const config_t *config);
void instr_00EE(chip8_t *chip8, const config_t *config);
void instr_0NNN(chip8_t *chip8, const config_t *config);

void instr_1NNN(chip8_t *chip8, const config_t *config);
void instr_2NNN(chip8_t *chip8, const config_t *config);
void instr_3XNN(chip8_t *chip8, const config_t *config);
void instr_4XNN(chip8_t *chip8, const config_t *config);
void instr_5XY0(chip8_t *chip8, const config_t *config);
void instr_6XNN(chip8_t *chip8, const config_t *config);
void instr_7XNN(chip8_t *chip8, const config_t *config);

void instr_8XYN(chip8_t *chip8, const config_t *config);
void instr_8XY0(chip8_t *chip8, const config_t *config);
void instr_8XY1(chip8_t *chip8, const config_t *config);
void instr_8XY2(chip8_t *chip8, const config_t *config);
void instr_8XY3(chip8_t *chip8, const config_t *config);
void instr_8XY4(chip8_t *chip8, const config_t *config);
void instr_8XY5(chip8_t *chip8, const config_t *config);
void instr_8XY6(chip8_t *chip8, const config_t *config);
void instr_8XY7(chip8_t *chip8, const config_t *config);
void instr_8XYE(chip8_t *chip8, const config_t *config);

void instr_9XY0(chip8_t *chip8, const config_t *config);
void instr_ANNN(chip8_t *chip8, const config_t *config);
void instr_BNNN(chip8_t *chip8, const config_t *config);
void instr_CXNN(chip8_t *chip8, const config_t *config);
void instr_DXYN(chip8_t *chip8, const config_t *config);

void instr_EXNN(chip8_t *chip8, const config_t *config);
void instr_EX9E(chip8_t *chip8, const config_t *config);
void instr_EXA1(chip8_t *chip8, const config_t *config);

void instr_FXNN(chip8_t *chip8, const config_t *config);
void instr_FX07(chip8_t *chip8, const config_t *config);
void instr_FX0A(chip8_t *chip8, const config_t *config);
void instr_FX15(chip8_t *chip8, const config_t *config);
void instr_FX18(chip8_t *chip8, const config_t *config);
void instr_FX1E(chip8_t *chip8, const config_t *config);
void instr_FX29(chip8_t *chip8, const config_t *config);
void instr_FX33(chip8_t *chip8, const config_t *config);
void instr_FX55(chip8_t *chip8, const config_t *config);
void instr_FX65(chip8_t *chip8, const config_t *config);

#endif