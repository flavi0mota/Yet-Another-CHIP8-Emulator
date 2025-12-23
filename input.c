#include <stdbool.h>

#include <SDL2/SDL.h>

#include "input.h"

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
