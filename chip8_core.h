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

#ifndef C8_CHIP8_CORE_H
#define C8_CHIP8_CORE_H

#include <stdint.h>
#include <stdbool.h>

#define C8_MEMORY_SIZE 0x1000
#define C8_V_REGISTERS 16
#define C8_STACK_SIZE 16
#define C8_DISPLAY_MAX_HEIGHT 64
#define C8_DISPLAY_MAX_WIDTH 128
#define C8_DISPLAY_HEIGHT 32
#define C8_DISPLAY_WIDTH 64
#define C8_KEY_NUM 16
#define C8_TIMER_FREQ_HZ 60
#define C8_PROGRAM_MEMORY_START 0x200
#define C8_PROGRAM_MEMORY_SIZE (C8_MEMORY_SIZE - C8_PROGRAM_MEMORY_START)

typedef struct {
    uint8_t memory[C8_MEMORY_SIZE]; 
    uint8_t register_V[C8_V_REGISTERS];
    uint16_t register_I;
    uint8_t register_delay_timer;
    uint8_t register_sound_timer;
    uint16_t program_counter;
    uint16_t stack[C8_STACK_SIZE];
    uint8_t stack_pointer;
    /* SuperChip allows for larger display, so maximum possible 
     * display size is allocated and current dimensions are stored */
    uint8_t display[C8_DISPLAY_MAX_WIDTH * C8_DISPLAY_MAX_HEIGHT];
    uint8_t display_height;
    uint8_t display_width;
    bool update_display;
    uint8_t input_keys[C8_KEY_NUM];
    /* This variable starts off with a value of -1.
     * When we need to wait for keyboard input and place the entered value
     * into a specified V register, this variable is set equal to the V register
     * where the input should be placed. After the keyboard input has been read and
     * the V register updated this variable is set back to -1. */
    int8_t wait_key_V_reg;
} Chip8;

void c8_init(Chip8 *chip8);
void c8_run_cycle(Chip8 *chip8);
void c8_update_timers(Chip8 *chip8);

#endif
