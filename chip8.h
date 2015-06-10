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

#ifndef C8_CHIP8_H
#define C8_CHIP8_H

#include <stdio.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define C8_LOG_ERROR(msg,...) fprintf(stderr, "ERROR:%s:%d: " msg "\n", __FILE__, __LINE__, ##__VA_ARGS__)

/* Values that can be set from the command line, see help message for explanation */
typedef struct {
    const char *rom_file_path;
    int scale_factor;
    int instr_per_sec;
} Chip8Option;

#endif
