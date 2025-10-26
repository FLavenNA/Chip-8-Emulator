#include "instruction_tables.h"

// TODO: U used func callbacks to manage all 
// But maybe u made it complicated ? Go back to switch case ? 

// Opcode Function Callback Table
instruction_func_t opcode_table[16] = {
    instr_0NNN, // 0x0***
    instr_1NNN, // 0x1***
    instr_2NNN, // 0x2***
    instr_3XNN, // 0x3***
    instr_4XNN, // 0x4***
    instr_5XY0, // 0x5***
    instr_6XNN, // 0x6***
    instr_7XNN, // 0x7***
    instr_8XYN, // 0x8***
    instr_9XY0, // 0x9***
    instr_ANNN, // 0xA***
    instr_BNNN, // 0xB***
    instr_CXNN, // 0xC***
    instr_DXYN, // 0xD***
    instr_EXNN, // 0xE***
    instr_FXNN  // 0xF***
};

// 0NNN Function Callback Table
instruction_func_t table_0NNN[0x100] = {
    [0xE0] = instr_00E0, // 0x00E0
    [0xEE] = instr_00EE, // 0x00EE
};

// 8XYN Function Callback Table
instruction_func_t table_8XYN[16] = {
    instr_8XY0,  // 0x8XY0
    instr_8XY1,  // 0x8XY1
    instr_8XY2,  // 0x8XY2
    instr_8XY3,  // 0x8XY3
    instr_8XY4,  // 0x8XY4
    instr_8XY5,  // 0x8XY5
    instr_8XY6,  // 0x8XY6
    instr_8XY7,  // 0x8XY7
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    instr_8XYE,   // 0x8XYE
    NULL
};

// EXNN Function Callback Table
instruction_func_t table_EXNN[0x100] = {
    [0x9E] = instr_EX9E,  // 0xEX9E
    [0xA1] = instr_EXA1,  // 0xEXA1
};

// FXNN Function Callback Table
instruction_func_t table_FXNN[0x100] = {
    [0x07] = instr_FX07,  // 0xFX07
    [0x0A] = instr_FX0A,  // 0xFX0A
    [0x15] = instr_FX15,  // 0xFX15
    [0x18] = instr_FX18,  // 0xFX18
    [0x1E] = instr_FX1E,  // 0xFX1E
    [0x29] = instr_FX29,  // 0xFX29
    [0x33] = instr_FX33,  // 0xFX33
    [0x55] = instr_FX55,  // 0xFX55
    [0x65] = instr_FX65   // 0xFX65
};
