#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include <SDL2/SDL.h>

#include "chip8.h"


//efeito de "flick" de monitores antigos
uint32_t color_lerp(const uint32_t start_color, const uint32_t end_color, const float t) {
  const uint8_t s_r = (start_color >> 24) & 0xFF;
  const uint8_t s_g = (start_color >> 16) & 0xFF;
  const uint8_t s_b = (start_color >> 8)  & 0xFF;
  const uint8_t s_a = (start_color >> 0)  & 0xFF;

  const uint8_t e_r = (end_color >> 24)   & 0xFF;
  const uint8_t e_g = (end_color >> 16)   & 0xFF;
  const uint8_t e_b = (end_color >> 8)    & 0xFF;
  const uint8_t e_a = (end_color >> 0)    & 0xFF;

  const uint8_t ret_r = ((1-t)*(s_r)) + (t*e_r);
  const uint8_t ret_b = ((1-t)*(s_b)) + (t*e_b);
  const uint8_t ret_g = ((1-t)*(s_g)) + (t*e_g);
  const uint8_t ret_a = ((1-t)*(s_a)) + (t*e_a);

  return (ret_r << 24) | (ret_b << 16) | (ret_g << 8) | (ret_a);
}

void audio_callback(void *userdata, uint8_t *stream, int len) {
  config_t *config = (config_t *)userdata;
  
  int16_t *audio_data = (int16_t *)stream;
  static uint32_t running_sample_index = 0;
  const int32_t square_wave_period = config->audio_sample_rate / config->square_wave_freq;
  const int32_t half_square_wave_period = square_wave_period / 2;

  for (int i = 0; i < len/2; i++) {
    audio_data[i] = ((running_sample_index++ / half_square_wave_period) % 2) ?
                    config->volume :
                    -config->volume;
}
}

bool init_sdl(sdl_t *sdl, config_t *config) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
    SDL_Log("Could not Initialize SDL: %s\n", SDL_GetError());
    return false;
  }

  sdl->window = SDL_CreateWindow("EMULADOR CHIP8", 
                                 SDL_WINDOWPOS_CENTERED,
                                 SDL_WINDOWPOS_CENTERED,
                                 config->window_width * config->scale_factor,
                                 config->window_height * config->scale_factor,
                                 0);

  if (!sdl->window) {
    SDL_Log("Could not initialize window: %s\n", SDL_GetError());
    return false;
  }

  sdl->renderer = SDL_CreateRenderer(sdl->window, -1, SDL_RENDERER_ACCELERATED);

  if (!sdl->renderer) {
    SDL_Log("Could not initialize renderer: %s\n", SDL_GetError());
    return false;
  }

  sdl->want = (SDL_AudioSpec){
    .freq = 44100,
    .format = AUDIO_S16LSB,
    .channels = 1,
    .samples = 512,
    .callback = audio_callback,
    .userdata = config,
  };

  sdl->dev = SDL_OpenAudioDevice(NULL, 0, &sdl->want, &sdl->have, 0);

  if (sdl->dev == 0) {
    SDL_Log("Could not initiate audio device: %s\n", SDL_GetError());
    return false;
  }

  if ((sdl->want.format != sdl->have.format) || (sdl->want.channels != sdl->have.channels)) {
    SDL_Log("Could not get audio spec: %s\n", SDL_GetError());
    return false;
  }

  return true;
}

bool set_config_from_args(config_t* config, const int argc, char** argv) {
  *config = (config_t){
    .window_width = 64,
    .window_height = 32,
    .fg_color = 0xFFFFFFFF,
    .bg_color = 0x000000FF,
    .scale_factor = 20,
    .pixel_outlines = true,
    .insts_per_second = 600,
    .square_wave_freq = 440,
    .audio_sample_rate = 44100,
    .volume = 3000,
    .color_lerp_rate = 0.7,
    .current_extension = CHIP8
  };

  for (int i = 1; i < argc; i++) {
    (void)argv[i];

    if (strncmp(argv[i], "--scale-factor", strlen("--scale-factor")) == 0) {
      i++;
      config->scale_factor = (uint32_t)strtol(argv[i], NULL, 10);
    }
  }

  return true;
}

bool init_chip8(chip8_t *chip8, const config_t config, const char rom_name[]) {
    const uint32_t entry_point = 0x200; // CHIP8 Roms will be loaded to 0x200
    const uint8_t font[] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0,   // 0   
        0x20, 0x60, 0x20, 0x20, 0x70,   // 1  
        0xF0, 0x10, 0xF0, 0x80, 0xF0,   // 2 
        0xF0, 0x10, 0xF0, 0x10, 0xF0,   // 3
        0x90, 0x90, 0xF0, 0x10, 0x10,   // 4    
        0xF0, 0x80, 0xF0, 0x10, 0xF0,   // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0,   // 6
        0xF0, 0x10, 0x20, 0x40, 0x40,   // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0,   // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0,   // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90,   // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0,   // B
        0xF0, 0x80, 0x80, 0x80, 0xF0,   // C
        0xE0, 0x90, 0x90, 0x90, 0xE0,   // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0,   // E
        0xF0, 0x80, 0xF0, 0x80, 0x80,   // F
    };

    // Initialize entire CHIP8 machine
    memset(chip8, 0, sizeof(chip8_t));

    // Load font 
    memcpy(&chip8->ram[0], font, sizeof(font));
   
    // Open ROM file
    FILE *rom = fopen(rom_name, "rb");
    if (!rom) {
        SDL_Log("Rom file %s is invalid or does not exist\n", rom_name);
        return false;
    }

    // Get/check rom size
    fseek(rom, 0, SEEK_END);
    const size_t rom_size = ftell(rom);
    const size_t max_size = sizeof chip8->ram - entry_point;
    rewind(rom);

    if (rom_size > max_size) {
        SDL_Log("Rom file %s is too big! Rom size: %llu, Max size allowed: %llu\n", 
                rom_name, (long long unsigned)rom_size, (long long unsigned)max_size);
        return false;
    }

    // Load ROM
    if (fread(&chip8->ram[entry_point], rom_size, 1, rom) != 1) {
        SDL_Log("Could not read Rom file %s into CHIP8 memory\n", 
                rom_name);
        return false;
    }
    fclose(rom);

    // Set chip8 machine defaults
    chip8->state = RUNNING;     // Default machine state to on/running
    chip8->PC = entry_point;    // Start program counter at ROM entry point
    chip8->rom_name = rom_name;
    chip8->stack_ptr = &chip8->stack[0];
    memset(&chip8->pixel_color[0], config.bg_color, sizeof chip8->pixel_color); // Init pixels to bg color

    return true;    // Success
}

void final_cleanup(const sdl_t sdl) {
  SDL_DestroyRenderer(sdl.renderer);
  SDL_DestroyWindow(sdl.window);
  SDL_CloseAudioDevice(sdl.dev);

  SDL_Quit();
}

void clear_screen(const sdl_t sdl, const config_t config) {
    const uint8_t r = (config.bg_color >> 24) & 0xFF;
    const uint8_t g = (config.bg_color >> 16) & 0xFF;
    const uint8_t b = (config.bg_color >>  8) & 0xFF;
    const uint8_t a = (config.bg_color >>  0) & 0xFF;

    SDL_SetRenderDrawColor(sdl.renderer, r, g, b, a);
    SDL_RenderClear(sdl.renderer);
}

void update_screen(const sdl_t sdl, const config_t config, chip8_t *chip8) {
    SDL_Rect rect = {.x = 0, .y = 0, .w = config.scale_factor, .h = config.scale_factor};

    const uint8_t bg_r = (config.bg_color >> 24) & 0xFF;
    const uint8_t bg_g = (config.bg_color >> 16) & 0xFF;
    const uint8_t bg_b = (config.bg_color >>  8) & 0xFF;
    const uint8_t bg_a = (config.bg_color >>  0) & 0xFF;

    for (uint32_t i = 0; i < sizeof chip8->display; i++) {
        rect.x = (i % config.window_width) * config.scale_factor;
        rect.y = (i / config.window_width) * config.scale_factor;

        if (chip8->display[i]) {
            if (chip8->pixel_color[i] != config.fg_color) {
                chip8->pixel_color[i] = color_lerp(chip8->pixel_color[i], 
                                                   config.fg_color, 
                                                   config.color_lerp_rate);
            }

            const uint8_t r = (chip8->pixel_color[i] >> 24) & 0xFF;
            const uint8_t g = (chip8->pixel_color[i] >> 16) & 0xFF;
            const uint8_t b = (chip8->pixel_color[i] >>  8) & 0xFF;
            const uint8_t a = (chip8->pixel_color[i] >>  0) & 0xFF;

            SDL_SetRenderDrawColor(sdl.renderer, r, g, b, a);
            SDL_RenderFillRect(sdl.renderer, &rect);
        
            if (config.pixel_outlines) {
                SDL_SetRenderDrawColor(sdl.renderer, bg_r, bg_g, bg_b, bg_a);
                SDL_RenderDrawRect(sdl.renderer, &rect);
            }

        } else {
            if (chip8->pixel_color[i] != config.bg_color) {
                // Lerp
                chip8->pixel_color[i] = color_lerp(chip8->pixel_color[i], 
                                                   config.bg_color, 
                                                   config.color_lerp_rate);
            }

            const uint8_t r = (chip8->pixel_color[i] >> 24) & 0xFF;
            const uint8_t g = (chip8->pixel_color[i] >> 16) & 0xFF;
            const uint8_t b = (chip8->pixel_color[i] >>  8) & 0xFF;
            const uint8_t a = (chip8->pixel_color[i] >>  0) & 0xFF;

            SDL_SetRenderDrawColor(sdl.renderer, r, g, b, a);
            SDL_RenderFillRect(sdl.renderer, &rect);
        }
    }

    SDL_RenderPresent(sdl.renderer);
}

// semelhante ao congelar da vm
bool save_estado_chip(const chip8_t *chip8, const char *filename) {
  FILE *file = fopen(filename, "wb"); // provavelmente vou settar isso no argumento

  if (!file) {
    SDL_Log("Falha em abrir o abrir o arquivo %s\n", filename);
    return false;
  }

  if (fwrite(chip8, sizeof(file), 1, file) != 1) {
    SDL_Log("Falha em escrever estado para %s", filename);
    return false;
  }

  fclose(file);

  return true;
}

bool carregar_estado_chip(chip8_t *chip8, const char *filename) {
  FILE *file = fopen(filename, "rb");

  if (!file) {
    SDL_Log("Não foi possível encontrar o save %s\n", filename);
    return false;
  }

  if (fread(chip8, sizeof(chip8_t), 1, file) != 1) {
    SDL_Log("Não foi possível ler o save %s\n", filename);
    fclose(file); //lê o arquivo corrompido se não liberar mem aqui

    return false;
  }

  fclose(file);

  return true;
}

void update_timers(const sdl_t sdl, chip8_t *chip8) {
    if (chip8->delay_timer > 0) 
        chip8->delay_timer--;

    if (chip8->sound_timer > 0) {
        chip8->sound_timer--;
        SDL_PauseAudioDevice(sdl.dev, 0); // Play sound
    } else {
        SDL_PauseAudioDevice(sdl.dev, 1); // Pause sound
    }
}
