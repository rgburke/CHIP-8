/*
 * Copyright (C) 2015 Richard Burke
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "chip8_core.h"
#include "chip8.h"

#define C8_REG_V_IDX(instruction) (((instruction) & 0x0F00) >> 8)
#define C8_REG_V2_IDX(instruction) (((instruction) & 0x00F0) >> 4)
#define C8_INSTR_VALUE(instruction) ((instruction) & 0x00FF)

static uint16_t c8_fetch_next_instruction(const Chip8 *);
static void execute_instruction(Chip8 *, uint16_t);

static const uint8_t c8_builtin_sprites[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0,
    0x20, 0x60, 0x20, 0x20, 0x70,
    0xF0, 0x10, 0xF0, 0x80, 0xF0,
    0xF0, 0x10, 0xF0, 0x10, 0xF0,
    0x90, 0x90, 0xF0, 0x10, 0x10,
    0xF0, 0x80, 0xF0, 0x10, 0xF0,
    0xF0, 0x80, 0xF0, 0x90, 0xF0,
    0xF0, 0x10, 0x20, 0x40, 0x40,
    0xF0, 0x90, 0xF0, 0x90, 0xF0,
    0xF0, 0x90, 0xF0, 0x10, 0xF0,
    0xF0, 0x90, 0xF0, 0x90, 0x90,
    0xE0, 0x90, 0xE0, 0x90, 0xE0,
    0xF0, 0x80, 0x80, 0x80, 0xF0,
    0xE0, 0x90, 0x90, 0x90, 0xE0,
    0xF0, 0x80, 0xF0, 0x80, 0xF0,
    0xF0, 0x80, 0xF0, 0x80, 0x80 
};

void c8_init(Chip8 *chip8)
{
    memset(chip8, 0, sizeof(Chip8));

    chip8->program_counter = C8_PROGRAM_MEMORY_START;
    chip8->display_height = C8_DISPLAY_HEIGHT;
    chip8->display_width = C8_DISPLAY_WIDTH;
    chip8->wait_key_V_reg = -1;

    memcpy(chip8->memory, c8_builtin_sprites, sizeof(c8_builtin_sprites));

    srand(time(NULL));
}

void c8_run_cycle(Chip8 *chip8)
{
    uint16_t instr = c8_fetch_next_instruction(chip8);
    execute_instruction(chip8, instr);
}

static uint16_t c8_fetch_next_instruction(const Chip8 *chip8)
{
    /* Instructions are 2 bytes long and stored most significant byte first */
    return chip8->memory[chip8->program_counter] << 8 | 
           chip8->memory[chip8->program_counter + 1];
}

static void execute_instruction(Chip8 *chip8, uint16_t instr)
{
    /* See http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#3.1 
     * for a description of CHIP-8 instructions */

    switch (instr & 0xF000) {
        case 0x0: {
            switch (instr) {
                case 0x00E0: {
                    memset(chip8->display, 0, sizeof(chip8->display));
                    chip8->update_display = true;
                    chip8->program_counter += 2;
                    return;
                }
                case 0x00EE: {
                    chip8->program_counter = chip8->stack[--chip8->stack_pointer];
                    chip8->program_counter += 2;
                    return;
                }
                default: {
                    break;
                }
            }
        }
        case 0x1000: {
            chip8->program_counter = instr & 0x0FFF;            
            return;
        }
        case 0x2000: {
            chip8->stack[chip8->stack_pointer++] = chip8->program_counter;
            chip8->program_counter = instr & 0x0FFF;            
            return;
        }
        case 0x3000: {
            if (chip8->register_V[C8_REG_V_IDX(instr)] == C8_INSTR_VALUE(instr)) {
                chip8->program_counter += 4;
            } else {
                chip8->program_counter += 2;
            }

            return;
        }
        case 0x4000: {
            if (chip8->register_V[C8_REG_V_IDX(instr)] != C8_INSTR_VALUE(instr)) {
                chip8->program_counter += 4;
            } else {
                chip8->program_counter += 2;
            }

            return;
        }
        case 0x5000: {
            if (chip8->register_V[C8_REG_V_IDX(instr)] == chip8->register_V[C8_REG_V2_IDX(instr)]) {
                chip8->program_counter += 4;
            } else {
                chip8->program_counter += 2;
            }

            return;
        }
        case 0x6000: {
            chip8->register_V[C8_REG_V_IDX(instr)] = C8_INSTR_VALUE(instr);
            chip8->program_counter += 2;
            return;
        }
        case 0x7000: {
            chip8->register_V[C8_REG_V_IDX(instr)] += C8_INSTR_VALUE(instr);
            chip8->program_counter += 2;
            return;
        }
        case 0x8000: {
            switch (instr & 0x000F) {
                case 0x0: {
                    chip8->register_V[C8_REG_V_IDX(instr)] = chip8->register_V[C8_REG_V2_IDX(instr)];
                    chip8->program_counter += 2;
                    return;
                }
                case 0x1: {
                    chip8->register_V[C8_REG_V_IDX(instr)] |= chip8->register_V[C8_REG_V2_IDX(instr)];
                    chip8->program_counter += 2;
                    return;
                }
                case 0x2: {
                    chip8->register_V[C8_REG_V_IDX(instr)] &= chip8->register_V[C8_REG_V2_IDX(instr)];
                    chip8->program_counter += 2;
                    return;
                }
                case 0x3: {
                    chip8->register_V[C8_REG_V_IDX(instr)] ^= chip8->register_V[C8_REG_V2_IDX(instr)];
                    chip8->program_counter += 2;
                    return;
                }
                case 0x4: {
                    uint8_t value1 = chip8->register_V[C8_REG_V_IDX(instr)];
                    uint8_t value2 = chip8->register_V[C8_REG_V2_IDX(instr)];

                    if (value1 > UINT8_MAX - value2) {
                        chip8->register_V[0xF] = 1;
                    } else {
                        chip8->register_V[0xF] = 0;
                    }

                    chip8->register_V[C8_REG_V_IDX(instr)] = value1 + value2;
                    chip8->program_counter += 2;
                    return;
                }
                case 0x5: {
                    uint8_t value1 = chip8->register_V[C8_REG_V_IDX(instr)];
                    uint8_t value2 = chip8->register_V[C8_REG_V2_IDX(instr)];

                    if (value1 > value2) {
                        chip8->register_V[0xF] = 1;
                    } else {
                        chip8->register_V[0xF] = 0;
                    }

                    chip8->register_V[C8_REG_V_IDX(instr)] = value1 - value2;
                    chip8->program_counter += 2;
                    return;
                }
                case 0x6: {
                    uint8_t value = chip8->register_V[C8_REG_V_IDX(instr)];

                    if (value & 0x1) {
                        chip8->register_V[0xF] = 1;
                    } else {
                        chip8->register_V[0xF] = 0;
                    }

                    chip8->register_V[C8_REG_V_IDX(instr)] /= 2;
                    chip8->program_counter += 2;
                    return;
                }
                case 0x7: {
                    uint8_t value1 = chip8->register_V[C8_REG_V_IDX(instr)];
                    uint8_t value2 = chip8->register_V[C8_REG_V2_IDX(instr)];

                    if (value2 > value1) {
                        chip8->register_V[0xF] = 1;
                    } else {
                        chip8->register_V[0xF] = 0;
                    }

                    chip8->register_V[C8_REG_V_IDX(instr)] = value2 - value1;
                    chip8->program_counter += 2;
                    return;
                }
                case 0xE: {
                    uint8_t value = chip8->register_V[C8_REG_V_IDX(instr)];

                    if (value & 0x80) {
                        chip8->register_V[0xF] = 1;
                    } else {
                        chip8->register_V[0xF] = 0;
                    }

                    chip8->register_V[C8_REG_V_IDX(instr)] *= 2;
                    chip8->program_counter += 2;
                    return;
                }
                default: {
                    break;
                }
            }
        }
        case 0x9000: {
            if (chip8->register_V[C8_REG_V_IDX(instr)] != chip8->register_V[C8_REG_V2_IDX(instr)]) {
                chip8->program_counter += 4;
            } else {
                chip8->program_counter += 2;
            }

            return;
        }
        case 0xA000: {
            chip8->register_I = (instr & 0x0FFF);
            chip8->program_counter += 2;
            return;
        }
        case 0xB000: {
            chip8->program_counter = (instr & 0x0FFF) + chip8->register_V[0];
            return;
        }
        case 0xC000: {
            chip8->register_V[C8_REG_V_IDX(instr)] = (rand() % UINT8_MAX) & C8_INSTR_VALUE(instr);
            chip8->program_counter += 2;
            return;
        }
        case 0xD000: {
            uint8_t x = chip8->register_V[C8_REG_V_IDX(instr)];
            uint8_t y = chip8->register_V[C8_REG_V2_IDX(instr)];
            uint8_t byte_num = (instr & 0x000F);
            uint8_t sprite_byte;
            uint16_t display_index;

            chip8->register_V[0xF] = 0;

            for (int byte = 0; byte < byte_num; byte++) {
                sprite_byte = chip8->memory[chip8->register_I + byte];

                for (int bit = 0; bit < 8; bit++) {
                    if (((0x80 >> bit) & sprite_byte) != 0) {
                        display_index = ((y + byte) * chip8->display_width) + x + bit; 

                        if (chip8->display[display_index] == 1) {
                            chip8->register_V[0xF] = 1; 
                        }

                        chip8->display[display_index] ^= 1;
                    }
                }
            }

            chip8->update_display = true;
            chip8->program_counter += 2;

            return;
        }
        case 0xE000: {
            switch (instr & 0x00FF) {
                case 0x9E: {
                    uint8_t key = chip8->register_V[C8_REG_V_IDX(instr)];

                    if (chip8->input_keys[key]) {
                        chip8->program_counter += 4;
                    } else {
                        chip8->program_counter += 2;
                    }

                    return;
                }
                case 0xA1: {
                    uint8_t key = chip8->register_V[C8_REG_V_IDX(instr)];

                    if (chip8->input_keys[key]) {
                        chip8->program_counter += 2;
                    } else {
                        chip8->program_counter += 4;
                    }

                    return;
                }
                default: {
                    break;
                }
            }
        }
        case 0xF000: {
            switch (instr & 0x00FF) {
                case 0x07: {
                    chip8->register_V[C8_REG_V_IDX(instr)] = chip8->register_delay_timer;
                    chip8->program_counter += 2;
                    return;
                }
                case 0x0A: {
                    chip8->wait_key_V_reg = C8_REG_V_IDX(instr);
                    chip8->program_counter += 2;
                    return;
                }
                case 0x15: {
                    chip8->register_delay_timer = chip8->register_V[C8_REG_V_IDX(instr)];
                    chip8->program_counter += 2;
                    return;
                }
                case 0x18: {
                    chip8->register_sound_timer = chip8->register_V[C8_REG_V_IDX(instr)];
                    chip8->program_counter += 2;
                    return;
                }
                case 0x1E: {
                    chip8->register_I += chip8->register_V[C8_REG_V_IDX(instr)];
                    chip8->program_counter += 2;
                    return;
                }
                case 0x29: {
                    chip8->register_I = (chip8->register_V[C8_REG_V_IDX(instr)] * 5);
                    chip8->program_counter += 2;
                    return;
                }
                case 0x33: {
                    uint8_t value = chip8->register_V[C8_REG_V_IDX(instr)];

                    chip8->memory[chip8->register_I] = value / 100;
                    chip8->memory[chip8->register_I + 1] = (value / 10) % 10;
                    chip8->memory[chip8->register_I + 2] = value % 10;

                    chip8->program_counter += 2;
                    return;
                }
                case 0x55: {
                    uint8_t v_reg_num = C8_REG_V_IDX(instr) + 1;

                    for (int k = 0; k < v_reg_num; k++) {
                        chip8->memory[chip8->register_I + k] = chip8->register_V[k];
                    }

                    chip8->program_counter += 2;
                    return;
                }
                case 0x65: {
                    uint8_t v_reg_num = C8_REG_V_IDX(instr) + 1;

                    for (int k = 0; k < v_reg_num; k++) {
                        chip8->register_V[k] = chip8->memory[chip8->register_I + k];
                    }

                    chip8->program_counter += 2;
                    return;
                }
                default: {
                    break;
                }
            }
        }
        default: {
             break;
        }
    }

    C8_LOG_ERROR("Unknown instruction %X", instr);
    chip8->program_counter += 2;
}

void c8_update_timers(Chip8 *chip8)
{
    if (chip8->register_delay_timer != 0) {
        chip8->register_delay_timer--;
    }

    if (chip8->register_sound_timer != 0) {
        chip8->register_sound_timer--;
    }
}
