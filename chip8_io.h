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

#ifndef C8_CHIP8_IO_H
#define C8_CHIP8_IO_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include "chip8.h"
#include "chip8_core.h"

struct Chip8IO;
typedef struct Chip8IO Chip8IO;

/* Sound and delay timers are updated in a separate timer thread.
 * This struct is passed as an argument to the timer function. */
typedef struct {
    Chip8IO *io;
    Chip8 *chip8;
} Chip8TimerArgs;

struct Chip8IO {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    /* Used to scale display by scale_factor */
    SDL_Rect draw_rect;
    SDL_TimerID delay_sound_timer;
    /* Binary semaphore used to control access to
     * the display and sound timers, which are accessed
     * from both the main and timer threads. */
    SDL_sem *timer_lock; 
    SDL_AudioDeviceID audio_dev;
    /* The display is converted into pixel representation
     * which is used to update the display */
    uint32_t *pixels;
    uint16_t win_width;
    uint16_t win_height;
    uint8_t scale_factor;
    uint32_t instruction_timer;
    uint16_t instr_per_sec;
    Chip8TimerArgs timer_args;
    bool audio_playing;
};

int io_init(Chip8IO *io, Chip8 *chip8, const Chip8Option *opt);
void io_free(Chip8IO *io);
uint32_t io_update_delay_sound_timers(uint32_t interval, void *param);
void io_update_display(Chip8IO *io, Chip8 *chip8);
void io_update_key_states(Chip8 *chip8, int *quit);
void io_reset_instruction_timer(Chip8IO *io);
void io_cycle_time_limit(const Chip8IO *io);
void io_lock_timer(Chip8IO *io);
void io_unlock_timer(Chip8IO *io);

#endif
