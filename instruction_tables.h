#ifndef INSTRUCTION_TABLES_H
#define INSTRUCTION_TABLES_H

#include "chip8.h"

typedef void (*instruction_func_t)(chip8_t *chip8, const config_t *config);

// --- Declarations (no actual data here) ---
extern instruction_func_t opcode_table[16];
extern instruction_func_t table_0NNN[0x100];
extern instruction_func_t table_8XYN[16];
extern instruction_func_t table_EXNN[0x100];
extern instruction_func_t table_FXNN[0x100];

// (called once in chip8_init() to fill tables)
void init_subtables(void);

#endif