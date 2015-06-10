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

#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <getopt.h>
#include "chip8.h"
#include "chip8_core.h"
#include "chip8_io.h"

#define C8_INSTR_PER_SEC_DEFAULT 300
#define C8_INSTR_PER_SEC_MIN 1
#define C8_SCALE_FACTOR_DEFAULT 8
#define C8_SCALE_FACTOR_MIN 1
#define C8_SCALE_FACTOR_MAX 16

static bool c8_parse_args(Chip8Option *opt, int argc, char *argv[]);
static void c8_print_usage(void);
static bool c8_parse_int(const char *string_value, int *int_ptr);
static bool c8_load(Chip8 *chip8, const char *rom_file_path);

int main(int argc, char *argv[])
{
    Chip8Option opt = {
        .rom_file_path = NULL,
        .scale_factor = C8_SCALE_FACTOR_DEFAULT,
        .instr_per_sec = C8_INSTR_PER_SEC_DEFAULT
    };

    if (!c8_parse_args(&opt, argc, argv)) {
        c8_print_usage();
        return 1;
    }

    Chip8 chip8;

    c8_init(&chip8);

    if (!c8_load(&chip8, opt.rom_file_path)) {
        return false;
    }

    Chip8IO io;

    if (!io_init(&io, &chip8, &opt)) {
        return 1;
    }

    int quit = 0;

    while (!quit) {
        io_reset_instruction_timer(&io);
        io_lock_timer(&io);
        c8_run_cycle(&chip8);
        io_unlock_timer(&io);
        io_update_display(&io, &chip8);
        io_update_key_states(&chip8, &quit);
        io_cycle_time_limit(&io);
    }

    io_free(&io);

    return 0;
}

static bool c8_parse_args(Chip8Option *opt, int argc, char *argv[])
{
    struct option chip8_options[] = {
        { "help"        , no_argument      , 0, 'h' },
        { "instr-rate"  , optional_argument, 0, 'r' },
        { "scale-factor", optional_argument, 0, 's' },
        { 0, 0, 0, 0 }
    };

    int ch;

    while ((ch = getopt_long(argc, argv, "hs:r:", chip8_options, NULL)) != -1) {
        switch (ch) {
            case 'h': {
                c8_print_usage();
                exit(0);
            }
            case 'r': {
                if (!c8_parse_int(optarg, &opt->instr_per_sec) ||
                    opt->instr_per_sec < C8_INSTR_PER_SEC_MIN) {

                    fprintf(stderr,
                            "Invalid value passed for instr-rate: %s, "
                            "instr-rate must be an integer greater than %d\n",
                            optarg, C8_INSTR_PER_SEC_MIN - 1);

                    return false;
                }

                break;
            }
            case 's': {
                if (!c8_parse_int(optarg, &opt->scale_factor) ||
                    opt->scale_factor < C8_SCALE_FACTOR_MIN ||
                    opt->scale_factor > C8_SCALE_FACTOR_MAX) {

                    fprintf(stderr,
                            "Invalid value passed for scale-factor: %s, "
                            "scale-factor must be an integer between %d and %d inclusive\n",
                            optarg, C8_SCALE_FACTOR_MIN, C8_SCALE_FACTOR_MAX);

                    return false;
                }

                break;
            }
            case '?': {
                return false;  
            }
            default: {
                return false;
            }
        } 
    }

    if (optind < argc) {
        opt->rom_file_path = argv[optind];
    } else {
        fprintf(stderr, "No ROM file path provided\n");
        return false;
    }
    
    return true;
}

static void c8_print_usage(void)
{
    const char *help_msg = 
"\n\
CHIP-8 Interpreter\n\
\n\
Usage:\n\
chip8 [OPTIONS] ROMFILE\n\
\n\
ROMFILE:\n\
File path to a CHIP-8 ROM (required).\n\
\n\
OPTIONS:\n\
-h, --help                   Print this message.\n\
-r, --instr-rate=RATE        Run (roughly) RATE instructions per second.\n\
                             Default: %d, Min: %d.\n\
-s, --scale-factor=FACTOR    Scale display resolution by FACTOR.\n\
                             Default: %d, Min: %d, Max: %d.\n\
\n\
";

    printf(help_msg, C8_INSTR_PER_SEC_DEFAULT, C8_INSTR_PER_SEC_MIN, 
           C8_SCALE_FACTOR_DEFAULT, C8_SCALE_FACTOR_MIN, C8_SCALE_FACTOR_MAX);
}

static bool c8_parse_int(const char *string_value, int *int_ptr)
{
    if (string_value == NULL || int_ptr == NULL) {
        return false;
    }

    char *end_ptr;
    errno = 0;

    long val = strtol(string_value, &end_ptr, 10);

    if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) ||
        (errno != 0 && val == 0) || end_ptr == string_value) {
        return false;
    }

    if (val > INT_MAX || val < INT_MIN) {
        return false;
    }

    *int_ptr = (int)val;

    return true;
}

static bool c8_load(Chip8 *chip8, const char *rom_file_path)
{
    FILE *rom_file = fopen(rom_file_path, "rb");

    if (rom_file == NULL) {
        fprintf(stderr, "Unable to open file %s for reading - %s\n", 
                        rom_file_path, strerror(errno));
        return false;
    }

    fseek(rom_file, 0, SEEK_END);
    long rom_size = ftell(rom_file);
    fseek(rom_file, 0, SEEK_SET);

    if (ferror(rom_file) || rom_size < 0) {
        fprintf(stderr, "Unable to determine size of file %s - %s\n", 
                        rom_file_path, strerror(errno));
        return false;
    } else if (rom_size > C8_PROGRAM_MEMORY_SIZE) {
        fprintf(stderr, "Size of ROM file %s exceeds CHIP-8 "
                        "program memory space\n", rom_file_path);
        return false;
    }

    size_t read = fread(chip8->memory + C8_PROGRAM_MEMORY_START, 
                        1, C8_PROGRAM_MEMORY_SIZE, rom_file);

    bool error = ferror(rom_file);

    fclose(rom_file);

    if (error) {
        fprintf(stderr, "Error when reading file %s - %s\n",
                        rom_file_path, strerror(errno));
        return false;
    } else if ((long)read != rom_size) {
        fprintf(stderr, "Reading ROM file %s data failed", rom_file_path);
        return false;
    }

    return true;
}

