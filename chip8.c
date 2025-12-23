#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include <SDL2/SDL.h>
#define STACK_SIZE 12

typedef struct {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_AudioSpec want, have;
  SDL_AudioDeviceID dev;
} sdl_t;

typedef enum {
  QUIT,
  RUNNING,
  PAUSE,
} emulator_state_t;

typedef enum {
  CHIP8,
  SUPERCHIP,
  XOCHIP,
} extension_t;

typedef struct {
  uint32_t window_width;
  uint32_t window_height;
  uint32_t fg_color;
  uint32_t bg_color;
  uint32_t scale_factor;
  bool pixel_outlines;
  uint32_t insts_per_second;
  uint32_t square_wave_freq;
  uint32_t audio_sample_rate;
  int16_t volume;
  float color_lerp_rate;
  extension_t current_extension;
} config_t;

typedef struct {
  uint16_t opcode;
  uint16_t NNN;
  uint8_t NN;
  uint8_t N;
  uint8_t X;
  uint8_t Y;
} instruction_t;

typedef struct {
  emulator_state_t state;
  uint8_t ram[4096];
  bool display[64*32];
  uint32_t pixel_color[64*32];
  uint16_t stack[STACK_SIZE];
  uint16_t *stack_ptr;
  uint8_t V[16]; //reg de dados
  uint16_t I;
  uint16_t PC;
  uint8_t delay_timer;
  uint8_t sound_timer;
  bool keypad[16];
  const char *rom_name;
  instruction_t inst;
  bool draw;
} chip8_t;


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
    const uint8_t ret_g = ((1-t)*(s_g)) + (t*e_g);
    const uint8_t ret_b = ((1-t)*(s_b)) + (t*e_b);
    const uint8_t ret_a = ((1-t)*(s_a)) + (t*e_a);

    return (ret_r << 24) | (ret_g << 16) | (ret_b << 8) | (ret_a);
}

void audio_callback(void *userdata, uint8_t *stream, int len) {
    config_t *config = (config_t *)userdata;
    
    int16_t *audio_data = (int16_t *)stream;
    static uint32_t running_sample_index = 0;
    const int32_t square_wave_period = config->audio_sample_rate / config->square_wave_freq;
    const int32_t half_square_wave_period = square_wave_period / 2;
    
    for (int i = 0; i < len/2; i++) {
        audio_data[i] = ((running_sample_index / half_square_wave_period) % 2) ?
                        config->volume : -config->volume;
        
        
        running_sample_index = (running_sample_index + 1) % square_wave_period;
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
    FILE *file = fopen(filename, "wb");
    if (!file) return false;

    if (fwrite(chip8, sizeof(chip8_t), 1, file) != 1) {
        fclose(file);
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

//hora de codar o teclado

/*
CHIP8 Keypad  QWERTY 
123C          1234
456D          qwer
789E          asdf
A0BF          zxcv

colinha que o home nao e de ferro
*/

void handle_input(chip8_t *chip8, config_t *config) {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
      switch (event.type) {
          case SDL_QUIT:
              // Exit window; End program
              chip8->state = QUIT; // Will exit main emulator loop
              break;

          case SDL_KEYDOWN:
              switch (event.key.keysym.sym) {
                  case SDLK_ESCAPE:
                      // Escape key; Exit window & End program
                      chip8->state = QUIT;
                      break;
                      
                  case SDLK_SPACE:
                      // Space bar
                      if (chip8->state == RUNNING) {
                          chip8->state = PAUSE;  // Pause
                          puts("==== PAUSED ====");
                      } else {
                          chip8->state = RUNNING; // Resume
                      }
                      break;

                  case SDLK_EQUALS:
                      // '=': Reset CHIP8 machine for the current ROM
                      init_chip8(chip8, *config, chip8->rom_name);
                      break;

                  case SDLK_j:
                      // 'j': Decrease color lerp rate
                      if (config->color_lerp_rate > 0.1)
                          config->color_lerp_rate -= 0.1;
                      break;

                  case SDLK_k:
                      // 'k': Increase color lerp rate
                      if (config->color_lerp_rate < 1.0)
                          config->color_lerp_rate += 0.1;
                      break;

                  case SDLK_o:
                      // 'o': Decrease Volume
                      if (config->volume > 0)
                          config->volume -= 500;
                      break;

                  case SDLK_p:
                      // 'p': Increase Volume
                      if (config->volume < INT16_MAX)
                          config->volume += 500;
                      break;

                  // Save state on F5
                  case SDLK_F5:
                      if (save_estado_chip(chip8, "save_state.bin")) {
                          puts("State saved successfully.");
                      } else {
                          puts("Failed to save state.");
                      }
                      break;

                  // Load state on F9
                  case SDLK_F9:
                      if (carregar_estado_chip(chip8, "save_state.bin")) {
                          puts("State loaded successfully.");
                      } else {
                          puts("Failed to load state.");
                      }
                      break;

                  // Map qwerty keys to CHIP8 keypad
                  case SDLK_1: chip8->keypad[0x1] = true; break;
                  case SDLK_2: chip8->keypad[0x2] = true; break;
                  case SDLK_3: chip8->keypad[0x3] = true; break;
                  case SDLK_4: chip8->keypad[0xC] = true; break;

                  case SDLK_q: chip8->keypad[0x4] = true; break;
                  case SDLK_w: chip8->keypad[0x5] = true; break;
                  case SDLK_e: chip8->keypad[0x6] = true; break;
                  case SDLK_r: chip8->keypad[0xD] = true; break;

                  case SDLK_a: chip8->keypad[0x7] = true; break;
                  case SDLK_s: chip8->keypad[0x8] = true; break;
                  case SDLK_d: chip8->keypad[0x9] = true; break;
                  case SDLK_f: chip8->keypad[0xE] = true; break;

                  case SDLK_z: chip8->keypad[0xA] = true; break;
                  case SDLK_x: chip8->keypad[0x0] = true; break;
                  case SDLK_c: chip8->keypad[0xB] = true; break;
                  case SDLK_v: chip8->keypad[0xF] = true; break;

                  default: break;
                      
              }
              break; 

          case SDL_KEYUP:
              switch (event.key.keysym.sym) {
                  // qwerty to CHIP8 keypad
                  case SDLK_1: chip8->keypad[0x1] = false; break;
                  case SDLK_2: chip8->keypad[0x2] = false; break;
                  case SDLK_3: chip8->keypad[0x3] = false; break;
                  case SDLK_4: chip8->keypad[0xC] = false; break;

                  case SDLK_q: chip8->keypad[0x4] = false; break;
                  case SDLK_w: chip8->keypad[0x5] = false; break;
                  case SDLK_e: chip8->keypad[0x6] = false; break;
                  case SDLK_r: chip8->keypad[0xD] = false; break;

                  case SDLK_a: chip8->keypad[0x7] = false; break;
                  case SDLK_s: chip8->keypad[0x8] = false; break;
                  case SDLK_d: chip8->keypad[0x9] = false; break;
                  case SDLK_f: chip8->keypad[0xE] = false; break;

                  case SDLK_z: chip8->keypad[0xA] = false; break;
                  case SDLK_x: chip8->keypad[0x0] = false; break;
                  case SDLK_c: chip8->keypad[0xB] = false; break;
                  case SDLK_v: chip8->keypad[0xF] = false; break;

                  default: break;
              }
              break;

          default:
              break;
      }
  }
}

void emulate_instruction(chip8_t *chip8, const config_t config) {
    bool carry;   // valor carry flag/VF

    // Get next opcode from ram 
    chip8->inst.opcode = (chip8->ram[chip8->PC] << 8) | chip8->ram[chip8->PC+1];
    chip8->PC += 2;

    chip8->inst.NNN = chip8->inst.opcode & 0x0FFF;
    chip8->inst.NN = chip8->inst.opcode & 0x0FF;
    chip8->inst.N = chip8->inst.opcode & 0x0F;
    chip8->inst.X = (chip8->inst.opcode >> 8) & 0x0F;
    chip8->inst.Y = (chip8->inst.opcode >> 4) & 0x0F;

    if (chip8->PC >= 4095) {
        SDL_Log("PC fora do limite: %04X\n", chip8->PC);
        chip8->state = QUIT;
        return;
    }

    // Emulate opcode
    switch ((chip8->inst.opcode >> 12) & 0x0F) {
        case 0x00:
            if (chip8->inst.NN == 0xE0) {
                // 0x00E0: Clear
                memset(&chip8->display[0], false, sizeof chip8->display);
                chip8->draw = true;
            } else if (chip8->inst.NN == 0xEE) {
                // 0x00EE: Retorna de subrotina
                chip8->PC = *--chip8->stack_ptr;

            } else {
            }

            break;

        case 0x01:
            // 0x1NNN: Pulo para NNN
            chip8->PC = chip8->inst.NNN;
            break;

        case 0x02:
            // 0x2NNN: subrotina em NNN
            *chip8->stack_ptr++ = chip8->PC;  
            chip8->PC = chip8->inst.NNN;
            break;

        case 0x03:
            // 0x3XNN: Check if VX == NN, if so, skip the next instruction
            if (chip8->V[chip8->inst.X] == chip8->inst.NN)
                chip8->PC += 2;
            break;

        case 0x04:
            // 0x4XNN: Check if VX != NN, if so, skip the next instruction
            if (chip8->V[chip8->inst.X] != chip8->inst.NN)
                chip8->PC += 2;       // Skip next opcode/instruction
            break;

        case 0x05:
            // 0x5XY0: Check if VX == VY, if so, skip the next instruction
            if (chip8->inst.N != 0) break; // Wrong opcode

            if (chip8->V[chip8->inst.X] == chip8->V[chip8->inst.Y])
                chip8->PC += 2;       // Skip next opcode/instruction
            
            break;

        case 0x06:
            // 0x6XNN: Set register VX to NN
            chip8->V[chip8->inst.X] = chip8->inst.NN;
            break;

        case 0x07:
            // 0x7XNN: Set register VX += NN
            chip8->V[chip8->inst.X] += chip8->inst.NN;
            break;

        case 0x08:
            switch(chip8->inst.N) {
                case 0:
                    // 0x8XY0: Set register VX = VY
                    chip8->V[chip8->inst.X] = chip8->V[chip8->inst.Y];
                    break;

                case 1:
                    // 0x8XY1: Set register VX |= VY
                    chip8->V[chip8->inst.X] |= chip8->V[chip8->inst.Y];
                    if (config.current_extension == CHIP8)
                        chip8->V[0xF] = 0;  // Reset VF to 0
                    break;

                case 2:
                    // 0x8XY2: Set register VX &= VY
                    chip8->V[chip8->inst.X] &= chip8->V[chip8->inst.Y];
                    if (config.current_extension == CHIP8)
                        chip8->V[0xF] = 0;  // Reset VF to 0
                    break;

                case 3:
                    // 0x8XY3: Set register VX ^= VY
                    chip8->V[chip8->inst.X] ^= chip8->V[chip8->inst.Y];
                    if (config.current_extension == CHIP8)
                        chip8->V[0xF] = 0;  // Reset VF to 0
                    break;

                case 4:
                    // 0x8XY4: Set register VX += VY, set VF to 1 if carry, 0 if not 
                    carry = ((uint16_t)(chip8->V[chip8->inst.X] + chip8->V[chip8->inst.Y]) > 255);

                    chip8->V[chip8->inst.X] += chip8->V[chip8->inst.Y];
                    chip8->V[0xF] = carry; 
                    break;

                case 5: 
                    // 0x8XY5: Set register VX -= VY, set VF to 1 if there is not a borrow (result is positive/0)
                    carry = (chip8->V[chip8->inst.Y] <= chip8->V[chip8->inst.X]);

                    chip8->V[chip8->inst.X] -= chip8->V[chip8->inst.Y];
                    chip8->V[0xF] = carry;
                    break;

                case 6:
                    // 0x8XY6: Set register VX >>= 1, store shifted off bit in VF
                    if (config.current_extension == CHIP8) {
                        carry = chip8->V[chip8->inst.Y] & 1;    // Use VY
                        chip8->V[chip8->inst.X] = chip8->V[chip8->inst.Y] >> 1; // Set VX = VY result
                    } else {
                        carry = chip8->V[chip8->inst.X] & 1;    // Use VX
                        chip8->V[chip8->inst.X] >>= 1;          // Use VX
                    }

                    chip8->V[0xF] = carry;
                    break;

                case 7:
                    // 0x8XY7: Set register VX = VY - VX, set VF to 1 if there is not a borrow (result is positive/0)
                    carry = (chip8->V[chip8->inst.X] <= chip8->V[chip8->inst.Y]);

                    chip8->V[chip8->inst.X] = chip8->V[chip8->inst.Y] - chip8->V[chip8->inst.X];
                    chip8->V[0xF] = carry;
                    break;

                case 0xE:
                    if (config.current_extension == CHIP8) { 
                        carry = (chip8->V[chip8->inst.Y] & 0x80) >> 7; // Use VY
                        chip8->V[chip8->inst.X] = chip8->V[chip8->inst.Y] << 1; // Set VX = VY result
                    } else {
                        carry = (chip8->V[chip8->inst.X] & 0x80) >> 7;  // VX
                        chip8->V[chip8->inst.X] <<= 1;                  // Use VX
                    }

                    chip8->V[0xF] = carry;
                    break;

                default:
                    // Opcode errado ou não existe
                    break;
            }
            break;

        case 0x09:
            // 0x9XY0: Check if VX != VY; Skip next instruction if so
            if (chip8->V[chip8->inst.X] != chip8->V[chip8->inst.Y])
                chip8->PC += 2;
            break;

        case 0x0A:
            // 0xANNN: Set index register I to NNN
            chip8->I = chip8->inst.NNN;
            break;

        case 0x0B:
            // 0xBNNN: Jump to V0 + NNN
            chip8->PC = chip8->V[0] + chip8->inst.NNN;
            break;

        case 0x0C:
            // 0xCXNN: VX = rand() % 256 & NN (bitwise AND)
            chip8->V[chip8->inst.X] = (rand() % 256) & chip8->inst.NN;
            break;

        case 0x0D: {
            // 0xDXYN: Draw N-height sprite at coords X,Y; Read from memory location I;
            //   Screen pixels are XOR'd with sprite bits, 
            //   VF (Carry flag) is set if any screen pixels are set off; This is useful
            //   for collision detection or other reasons.
            uint8_t X_coord = chip8->V[chip8->inst.X] % config.window_width;
            uint8_t Y_coord = chip8->V[chip8->inst.Y] % config.window_height;
            const uint8_t orig_X = X_coord; // Original X value

            chip8->V[0xF] = 0;  // Initialize carry flag to 0

            // Loop over all N rows of the sprite
            for (uint8_t i = 0; i < chip8->inst.N; i++) {
                // Get next byte/row of sprite data
                const uint8_t sprite_data = chip8->ram[chip8->I + i];
                X_coord = orig_X;   // Reset X for next row to draw

                for (int8_t j = 7; j >= 0; j--) {
                    // set carry flag
                    bool *pixel = &chip8->display[Y_coord * config.window_width + X_coord]; 
                    const bool sprite_bit = (sprite_data & (1 << j));

                    if (sprite_bit && *pixel) {
                        chip8->V[0xF] = 1;  
                    }

                    // XOR display pixel
                    *pixel ^= sprite_bit;

                    // Para de desenhar se bater no canto da tela
                    if (++X_coord >= config.window_width) break;
                }

                if (++Y_coord >= config.window_height) break;
            }
            chip8->draw = true; // atualiza tela no próximo tick 60hz
            break;
        }

        case 0x0E:
            if (chip8->inst.NN == 0x9E) {
                // 0xEX9E: Skip next instruction if key in VX is pressed
                if (chip8->keypad[chip8->V[chip8->inst.X]])
                    chip8->PC += 2;

            } else if (chip8->inst.NN == 0xA1) {
                // 0xEX9E: Skip next instruction if key in VX is not pressed
                if (!chip8->keypad[chip8->V[chip8->inst.X]])
                    chip8->PC += 2;
            }
            break;

        case 0x0F:
            switch (chip8->inst.NN) {
                case 0x0A: {
                    // 0xFX0A: VX = get_key(); guarda em VX
                    static bool any_key_pressed = false;
                    static uint8_t key = 0xFF;

                    for (uint8_t i = 0; key == 0xFF && i < sizeof chip8->keypad; i++) 
                        if (chip8->keypad[i]) {
                            key = i;   
                            any_key_pressed = true;
                            break;
                        }

                    
                    if (!any_key_pressed) chip8->PC -= 2; 
                    else {
                        // A key has been pressed, also wait until it is released to set the key in VX
                        if (chip8->keypad[key])     // "Busy loop" CHIP8 emulation until key is released
                            chip8->PC -= 2;
                        else {
                            chip8->V[chip8->inst.X] = key;     // VX = key 
                            key = 0xFF;                        // Reset key não encontrada
                            any_key_pressed = false;
                        }
                    }
                    break;
                }

                case 0x1E:
                    // 0xFX1E: I += VX; poe VX para reg I.
                    chip8->I += chip8->V[chip8->inst.X];
                    break;

                case 0x07:
                    // 0xFX07: VX = delay timer
                    chip8->V[chip8->inst.X] = chip8->delay_timer;
                    break;

                case 0x15:
                    // 0xFX15: delay timer = VX 
                    chip8->delay_timer = chip8->V[chip8->inst.X];
                    break;

                case 0x18:
                    // 0xFX18: sound timer = VX 
                    chip8->sound_timer = chip8->V[chip8->inst.X];
                    break;

                case 0x29:
                    // 0xFX29: Set register I to sprite location in memory for character in VX (0x0-0xF)
                    chip8->I = chip8->V[chip8->inst.X] * 5;
                    break;

                case 0x33: {
                    uint8_t bcd = chip8->V[chip8->inst.X]; 
                    chip8->ram[chip8->I+2] = bcd % 10;
                    bcd /= 10;
                    chip8->ram[chip8->I+1] = bcd % 10;
                    bcd /= 10;
                    chip8->ram[chip8->I] = bcd;
                    break;
                }

                case 0x55:
                    // 0xFX55: Register dump V0-VX inclusive to memory offset from I;
                    for (uint8_t i = 0; i <= chip8->inst.X; i++)  {
                        if (config.current_extension == CHIP8) 
                            chip8->ram[chip8->I++] = chip8->V[i]; // Incremento de reg I
                        else
                            chip8->ram[chip8->I + i] = chip8->V[i]; 
                    }
                    break;

                case 0x65:
                    // 0xFX65: Register load V0-VX inclusive from memory offset from I;
                    for (uint8_t i = 0; i <= chip8->inst.X; i++) {
                        if (config.current_extension == CHIP8) 
                            chip8->V[i] = chip8->ram[chip8->I++]; // Incremento de reg I
                        else
                            chip8->V[i] = chip8->ram[chip8->I + i];
                    }
                    break;

                default:
                    break;
            }
            break;
            
        default:
            break;  // Opcode inválido
    }
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

int main(int argc, char **argv) {
    if (argc < 2) {
       fprintf(stderr, "Usage: %s <rom_name>\n", argv[0]);
       exit(EXIT_FAILURE);
    }

    // Configuração inicial
    config_t config = {0};
    if (!set_config_from_args(&config, argc, argv)) exit(EXIT_FAILURE);

    sdl_t sdl = {0};
    if (!init_sdl(&sdl, &config)) exit(EXIT_FAILURE);

    // Setar a máquina
    chip8_t chip8 = {0};
    const char *rom_name = argv[1];
    if (!init_chip8(&chip8, config, rom_name)) exit(EXIT_FAILURE);

    clear_screen(sdl, config);

    srand(time(NULL));

    // Loop do emulador
    while (chip8.state != QUIT) {
        // suporte a teclado
        handle_input(&chip8, &config);

        if (chip8.state == PAUSE) continue;

        const uint64_t start_frame_time = SDL_GetPerformanceCounter();
        
        // Emulate CHIP8 Instructions for this emulator "frame" (60hz)
        for (uint32_t i = 0; i < config.insts_per_second / 60; i++) {
            emulate_instruction(&chip8, config);

            // If drawing on CHIP8, only draw 1 sprite this frame (display wait)
            if ((config.current_extension == CHIP8) && 
                (chip8.inst.opcode >> 12 == 0xD)) 
                break;  
        }

        // Tempo de execução da instrução
        const uint64_t end_frame_time = SDL_GetPerformanceCounter();

        // Delay 60hz/60fps (16.67ms)
        const double time_elapsed = (double)((end_frame_time - start_frame_time) * 1000) / SDL_GetPerformanceFrequency();

        SDL_Delay(16.67f > time_elapsed ? 16.67f - time_elapsed : 0);

        // Update window with changes every 60hz
        if (chip8.draw) {
          update_screen(sdl, config, &chip8);
          chip8.draw = false;
        }
        update_timers(sdl, &chip8);
    }
    final_cleanup(sdl); 

    exit(EXIT_SUCCESS);
}