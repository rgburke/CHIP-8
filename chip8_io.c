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
#include <math.h>
#include "chip8_io.h"
#include "chip8.h"

#define C8_CYCLE_TIME_MS (1000.0 / C8_TIMER_FREQ_HZ)
#define C8_AUDIO_AMPLITUDE 28000
#define C8_SAMPLE_FRAMES_FREQUENCY 44100
#define C8_AUDIO_FREQUENCY 880
#define C8_PI 3.14159265358979323846

static void io_wait_for_keypress(Chip8 *chip8, int *quit);
static int io_chip8_key_index(uint8_t keyboard_key);
static void io_audio_callback(void *user_data, uint8_t *audio_stream, int length);
static void io_update_audio_state(Chip8IO *io, uint8_t sound_timer);

static const SDL_Scancode io_keyboard_keys[C8_KEY_NUM] = {
    SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, SDL_SCANCODE_4,
    SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_R,
    SDL_SCANCODE_A, SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_F,
    SDL_SCANCODE_Z, SDL_SCANCODE_X, SDL_SCANCODE_C, SDL_SCANCODE_V
};

int io_init(Chip8IO *io, Chip8 *chip8, const Chip8Option *opt)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) {
        C8_LOG_ERROR("Unable to init SDL %s", SDL_GetError());
        return 0;
    }

    memset(io, 0, sizeof(Chip8IO));

    io->timer_args = (Chip8TimerArgs) {
        .chip8 = chip8,
        .io = io
    };

    io->scale_factor = opt->scale_factor;
    io->instr_per_sec = opt->instr_per_sec;

    uint16_t pixel_width = chip8->display_width * opt->scale_factor;
    uint16_t pixel_height = chip8->display_height * opt->scale_factor;

    io->window = SDL_CreateWindow("CHIP-8 Interpreter", SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED, pixel_width,
                                  pixel_height, 0);

    if (io->window == NULL) {
        C8_LOG_ERROR("Unable to create window %s", SDL_GetError());
        io_free(io);
        return 0;
    }

    io->renderer = SDL_CreateRenderer(io->window, -1, SDL_RENDERER_ACCELERATED);

    if (io->renderer == NULL) {
        C8_LOG_ERROR("Unable to create renderer %s", SDL_GetError());
        io_free(io);
        return 0;
    }

    io->texture = SDL_CreateTexture(io->renderer, SDL_PIXELFORMAT_ARGB8888,
                                    SDL_TEXTUREACCESS_STATIC, chip8->display_width,
                                    chip8->display_height);

    if (io->texture == NULL) {
        C8_LOG_ERROR("Unable to create texture %s", SDL_GetError());
        io_free(io);
        return 0;
    }

    io->delay_sound_timer = SDL_AddTimer(C8_CYCLE_TIME_MS, io_update_delay_sound_timers, &io->timer_args);

    if (io->delay_sound_timer == 0) {
        C8_LOG_ERROR("Unable to create timer %s", SDL_GetError());
        io_free(io);
        return 0;
    }

    io->timer_lock = SDL_CreateSemaphore(1);

    if (io->timer_lock == NULL) {
        C8_LOG_ERROR("Unable to create semaphore %s", SDL_GetError());
        io_free(io);
        return 0;
    }

    SDL_AudioSpec audio_want, audio_have;

    SDL_zero(audio_want);
    audio_want.freq = C8_SAMPLE_FRAMES_FREQUENCY;
    audio_want.format = AUDIO_S16SYS;
    audio_want.channels = 1;
    audio_want.samples = 2048;
    audio_want.callback = io_audio_callback;

    io->audio_dev = SDL_OpenAudioDevice(NULL, 0, &audio_want, &audio_have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);

    if (io->audio_dev == 0) {
        C8_LOG_ERROR("Unable to open audio: %s\n", SDL_GetError());
        io_free(io);
        return 0;
    }

    size_t pixel_bytes = sizeof(uint32_t) * chip8->display_width * chip8->display_height;
    io->pixels = malloc(pixel_bytes);

    if (io->pixels == NULL) {
        C8_LOG_ERROR("Unable to allocate %zu bytes for pixel memory", pixel_bytes);
        io_free(io);
        return 0;
    }

    memset(io->pixels, 0, pixel_bytes);

    io->draw_rect.w = chip8->display_width * opt->scale_factor;
    io->draw_rect.h = chip8->display_height * opt->scale_factor;

    return 1;
}

void io_free(Chip8IO *io)
{
    if (io == NULL) {
        return;
    }

    free(io->pixels);

    if (io->texture != NULL) {
        SDL_DestroyTexture(io->texture);
    }

    if (io->renderer != NULL) {
        SDL_DestroyRenderer(io->renderer);
    }

    if (io->window != NULL) {
        SDL_DestroyWindow(io->window);
    }

    if (io->delay_sound_timer != 0) {
        SDL_RemoveTimer(io->delay_sound_timer);
    }

    if (io->timer_lock != NULL) {
        SDL_DestroySemaphore(io->timer_lock);
    }

    if (io->audio_dev != 0) {
        SDL_CloseAudioDevice(io->audio_dev);
    }

    SDL_Quit();
}

uint32_t io_update_delay_sound_timers(uint32_t interval, void *param)
{
    Chip8TimerArgs *timer_args = param;
    io_lock_timer(timer_args->io);
    c8_update_timers(timer_args->chip8);
    uint8_t sound_timer = timer_args->chip8->register_sound_timer;
    io_unlock_timer(timer_args->io);
    io_update_audio_state(timer_args->io, sound_timer);
    return interval;
}

void io_update_display(Chip8IO *io, Chip8 *chip8)
{
    if (!chip8->update_display) {
        return;
    }

    for (int k = 0; k < (chip8->display_width * chip8->display_height); k++) {
        io->pixels[k] = chip8->display[k] ? 0x00FFFFFF : 0;
    }

    SDL_UpdateTexture(io->texture, NULL, io->pixels, chip8->display_width * sizeof(uint32_t));
    SDL_RenderClear(io->renderer);
    SDL_RenderCopy(io->renderer, io->texture, NULL, &io->draw_rect);
    SDL_RenderPresent(io->renderer);

    chip8->update_display = false;
}

void io_update_key_states(Chip8 *chip8, int *quit)
{
    SDL_Event event;

    if (chip8->wait_key_V_reg != -1) {
        io_wait_for_keypress(chip8, quit);
    }

    while (SDL_PollEvent(&event) != 0) {
        if (event.type == SDL_QUIT) {
            *quit = 1;
            return;
        }
    }

    const uint8_t *key_states = SDL_GetKeyboardState(NULL);

    for (int k = 0; k < C8_KEY_NUM; k++) {
        chip8->input_keys[k] = key_states[io_keyboard_keys[k]];
    }
}

static void io_wait_for_keypress(Chip8 *chip8, int *quit)
{
    SDL_Event event;

    while (SDL_WaitEvent(&event) != 0) {
        if (event.type == SDL_QUIT) {
            *quit = 1;
            return;
        } else if (event.type == SDL_KEYDOWN) {
            int key_index = io_chip8_key_index(event.key.keysym.scancode);

            if (key_index != -1) {
                chip8->register_V[chip8->wait_key_V_reg] = key_index;
                chip8->wait_key_V_reg = -1;
                return;
            }
        }
    }
}

static int io_chip8_key_index(uint8_t keyboard_key)
{
    for (int k = 0; k < C8_KEY_NUM; k++) {
        if (keyboard_key == io_keyboard_keys[k]) {
            return k;
        }
    }

    return -1;
}

void io_reset_instruction_timer(Chip8IO *io)
{
    io->instruction_timer = SDL_GetTicks();
}

void io_cycle_time_limit(const Chip8IO *io)
{
    uint32_t current_time = SDL_GetTicks();
    uint32_t cycle_time = 1000 / io->instr_per_sec;
    uint32_t sleep_time = 0;

    if (current_time - io->instruction_timer < cycle_time) {
        sleep_time = cycle_time - (current_time - io->instruction_timer); 
    }

    if (sleep_time > 0) {
        SDL_Delay(sleep_time);
    }
}

void io_lock_timer(Chip8IO *io)
{
    SDL_SemWait(io->timer_lock);
}

void io_unlock_timer(Chip8IO *io)
{
    SDL_SemPost(io->timer_lock);
}

static void io_audio_callback(void *user_data, uint8_t *audio_stream, int length)
{
    (void)user_data;
    static double v = 0;
    int16_t *stream = (int16_t *)audio_stream;
    length /= 2;

    for (int k = 0; k < length; k++) {
        stream[k] = C8_AUDIO_AMPLITUDE * sin(v * 2 * C8_PI / C8_SAMPLE_FRAMES_FREQUENCY);
        v += C8_AUDIO_FREQUENCY;
    }
}

static void io_update_audio_state(Chip8IO *io, uint8_t sound_timer)
{
    if (sound_timer > 0) {
        if (!io->audio_playing) {
            SDL_PauseAudioDevice(io->audio_dev, 0);
            io->audio_playing = true;
        }
    } else if (io->audio_playing) {
        SDL_PauseAudioDevice(io->audio_dev, 1);
        io->audio_playing = false;
    }
}
