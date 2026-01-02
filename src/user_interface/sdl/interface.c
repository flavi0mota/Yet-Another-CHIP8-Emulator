#include "user_interface/sdl/interface.h"
#include "user_interface/color_lerp.h"

// pause_menu.c
// Draws pause menu
static void pause_menu_user_interface_draw(struct UserInterface *user_interface);

// disassembling.c
// Prints instruction decoding info on screen in real time
static inline void disassembling_user_interface_draw(struct UserInterface *user_interface, struct EmulatedSystem *emulated_system);

#include "pause_menu.c"
#include "disassembling.c"

void emulator_user_interface_destroy(struct UserInterface *user_interface) {
    SDL_DestroyRenderer(user_interface->renderer);
    SDL_DestroyWindow(user_interface->window);
    SDL_CloseAudioDevice(user_interface->dev);
    SDL_Quit();
}

void emulator_user_interface_clear_screen(struct UserInterface *user_interface) {
    uint32_t bg_color = user_interface->bg_color;
    const uint8_t r = (bg_color >> 24) & 0xFF;
    const uint8_t g = (bg_color >> 16) & 0xFF;
    const uint8_t b = (bg_color >>  8) & 0xFF;
    const uint8_t a = (bg_color >>  0) & 0xFF;

    SDL_SetRenderDrawColor(user_interface->renderer, r, g, b, a);
    SDL_RenderClear(user_interface->renderer);
}

void emulator_user_interface_audio_callback(void *userdata, uint8_t *stream, int len) {
    struct UserInterface *user_interface = (struct UserInterface *)userdata;
    
    int16_t *audio_data = (int16_t *)stream;
    static uint32_t running_sample_index = 0;
    const int32_t square_wave_period = user_interface->audio_sample_rate / user_interface->square_wave_freq;
    const int32_t half_square_wave_period = square_wave_period / 2;
    
    for (int i = 0; i < len/2; i++) {
        audio_data[i] = ((running_sample_index / half_square_wave_period) % 2) ?
                        user_interface->volume : -user_interface->volume;
        
        
        running_sample_index = (running_sample_index + 1) % square_wave_period;
    }
}

bool emulator_user_interface_initialize(struct UserInterface *user_interface, struct EmulatedSystem *emulated_system) {
    *user_interface = (struct UserInterface){
        .desired_window_width = 64,
        .desired_window_height = 32,
        .fg_color = 0xFFFFFFFF,
        .bg_color = 0x000000FF,
        .scale_factor = 20,
        .pixel_outlines = true,
        .square_wave_freq = 440,
        .audio_sample_rate = 44100,
        .volume = 3000,
        .color_lerp_rate = 0.7,
    };

    // Init pixels to bg color
    memset(
        &user_interface->pixel_color[0],
        user_interface->bg_color,
        sizeof user_interface->pixel_color
    );

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        SDL_Log("Could not Initialize SDL: %s\n", SDL_GetError());
        return false;
    }

    user_interface->window = SDL_CreateWindow("EMULADOR CHIP8", 
                                 SDL_WINDOWPOS_CENTERED,
                                 SDL_WINDOWPOS_CENTERED,
                                 user_interface->desired_window_width * user_interface->scale_factor,
                                 user_interface->desired_window_height * user_interface->scale_factor,
                                 0);

    if (!user_interface->window) {
        SDL_Log("Could not initialize window: %s\n", SDL_GetError());
        return false;
    }

    user_interface->renderer = SDL_CreateRenderer(user_interface->window, -1, SDL_RENDERER_ACCELERATED);

    if (!user_interface->renderer) {
        SDL_Log("Could not initialize renderer: %s\n", SDL_GetError());
        return false;
    }

    user_interface->want = (SDL_AudioSpec){
        .freq = 44100,
        .format = AUDIO_S16LSB,
        .channels = 1,
        .samples = 512,
        .callback = emulator_user_interface_audio_callback,
        .userdata = emulated_system,
    };

    user_interface->dev = SDL_OpenAudioDevice(NULL, 0, &user_interface->want, &user_interface->have, 0);

    if (user_interface->dev == 0) {
        SDL_Log("Could not initiate audio device: %s\n", SDL_GetError());
        return false;
    }

    if ((user_interface->want.format != user_interface->have.format) || (user_interface->want.channels != user_interface->have.channels)) {
        SDL_Log("Could not get audio spec: %s\n", SDL_GetError());
        return false;
    }

    // TTF font

    TTF_Init();

    user_interface->font = TTF_OpenFont("/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf", 24);
    if (!user_interface->font) {
        printf("Font error: %s\n", TTF_GetError());
        return false;
    }

    user_interface->pause_menu.message_surface = TTF_RenderText_Blended_Wrapped(
        user_interface->font,
        "Game paused\n\nSpace: pause/resume\nF5: save state\nF9: load state", 
        (SDL_Color){255, 255, 255, 255},
        300
    );

    user_interface->pause_menu.message = SDL_CreateTextureFromSurface(
        user_interface->renderer,
        user_interface->pause_menu.message_surface
    );

    return true;
}

static void emulated_user_interface_draw(struct UserInterface *user_interface, struct EmulatedSystem *emulated_system) {
    uint64_t current_moment = SDL_GetTicks64();
    SDL_Rect rect;
    uint32_t bg_color = user_interface->bg_color;

    if (current_moment < user_interface->expected_moment_to_draw)
    { SDL_Delay(user_interface->expected_moment_to_draw - current_moment); }

    rect = (SDL_Rect){.x = 0, .y = 0, .w = user_interface->scale_factor, .h = user_interface->scale_factor};
    const uint8_t bg_r = (bg_color >> 24) & 0xFF;
    const uint8_t bg_g = (bg_color >> 16) & 0xFF;
    const uint8_t bg_b = (bg_color >>  8) & 0xFF;
    const uint8_t bg_a = (bg_color >>  0) & 0xFF;

    for (uint32_t i = 0; i < sizeof emulated_system->display; i++) {
        rect.x = (i % user_interface->desired_window_width) * user_interface->scale_factor;
        rect.y = (i / user_interface->desired_window_width) * user_interface->scale_factor;

        if (emulated_system->display[i]) {
            if (user_interface->pixel_color[i] != user_interface->fg_color) {
                user_interface->pixel_color[i] = user_interface_color_lerp(
                    user_interface->pixel_color[i], 
                    user_interface->fg_color, 
                    user_interface->color_lerp_rate
                );
            }

            const uint8_t r = (user_interface->pixel_color[i] >> 24) & 0xFF;
            const uint8_t g = (user_interface->pixel_color[i] >> 16) & 0xFF;
            const uint8_t b = (user_interface->pixel_color[i] >>  8) & 0xFF;
            const uint8_t a = (user_interface->pixel_color[i] >>  0) & 0xFF;

            SDL_SetRenderDrawColor(user_interface->renderer, r, g, b, a);
            SDL_RenderFillRect(user_interface->renderer, &rect);
        
            if (user_interface->pixel_outlines) {
                SDL_SetRenderDrawColor(user_interface->renderer, bg_r, bg_g, bg_b, bg_a);
                SDL_RenderDrawRect(user_interface->renderer, &rect);
            }

        } else {
            if (user_interface->pixel_color[i] != user_interface->bg_color) {
                // Lerp
                user_interface->pixel_color[i] = user_interface_color_lerp(
                    user_interface->pixel_color[i], 
                    user_interface->bg_color, 
                    user_interface->color_lerp_rate
                );
            }

            const uint8_t r = (user_interface->pixel_color[i] >> 24) & 0xFF;
            const uint8_t g = (user_interface->pixel_color[i] >> 16) & 0xFF;
            const uint8_t b = (user_interface->pixel_color[i] >>  8) & 0xFF;
            const uint8_t a = (user_interface->pixel_color[i] >>  0) & 0xFF;

            SDL_SetRenderDrawColor(user_interface->renderer, r, g, b, a);
            SDL_RenderFillRect(user_interface->renderer, &rect);
        }
    }
    disassembling_user_interface_draw(user_interface, emulated_system);
    SDL_RenderPresent(user_interface->renderer);
}

/*
CHIP8 Keypad  QWERTY 
123C          1234
456D          qwer
789E          asdf
A0BF          zxcv
*/

static void emulator_user_interface_handle_keyboard_event_key_down(struct UserInterface *user_interface, struct EmulatedSystem *emulated_system, SDL_Keycode key) {
  switch (key) {
      case SDLK_ESCAPE:
          // Escape key; Exit window & End program
          emulated_system->state = QUIT;
          break;
          
      case SDLK_SPACE:
          // Space bar
          if (emulated_system->state == RUNNING) {
              emulated_system->state = PAUSE;  // Pause
              puts("==== PAUSED ====");
          } else {
              emulated_system->state = RUNNING; // Resume
          }
          break;

      case SDLK_EQUALS:
          // '=': Reset CHIP8 machine for the current ROM
          //init_chip8(chip8, *config, chip8->rom_name);
          break;

      case SDLK_j:
          // 'j': Decrease color lerp rate
          if (user_interface->color_lerp_rate > 0.1)
              user_interface->color_lerp_rate -= 0.1;
          break;

      case SDLK_k:
          // 'k': Increase color lerp rate
          if (user_interface->color_lerp_rate < 1.0)
              user_interface->color_lerp_rate += 0.1;
          break;

      case SDLK_o:
          // 'o': Decrease Volume
          if (user_interface->volume > 0)
              user_interface->volume -= 500;
          break;

      case SDLK_p:
          // 'p': Increase Volume
          //if (emulator->user_interface->volume < INT16_MAX)
              //emulator->user_interface->volume += 500;
          break;

      // Save state
      case SDLK_F5:
          if (emulated_save_state(emulated_system, "save_state.bin")) {
              puts("State saved successfully.");
          } else {
              puts("Failed to save state.");
          }
          break;

      // Load state
      case SDLK_F9:
          if (emulated_load_state(emulated_system, "save_state.bin")) {
              puts("State loaded successfully.");
          } else {
              puts("Failed to load state.");
          }
          break;

      // Map qwerty keys to CHIP8 keypad
      case SDLK_1: emulated_system->keypad[0x1] = true; break;
      case SDLK_2: emulated_system->keypad[0x2] = true; break;
      case SDLK_3: emulated_system->keypad[0x3] = true; break;
      case SDLK_4: emulated_system->keypad[0xC] = true; break;

      case SDLK_q: emulated_system->keypad[0x4] = true; break;
      case SDLK_w: emulated_system->keypad[0x5] = true; break;
      case SDLK_e: emulated_system->keypad[0x6] = true; break;
      case SDLK_r: emulated_system->keypad[0xD] = true; break;

      case SDLK_a: emulated_system->keypad[0x7] = true; break;
      case SDLK_s: emulated_system->keypad[0x8] = true; break;
      case SDLK_d: emulated_system->keypad[0x9] = true; break;
      case SDLK_f: emulated_system->keypad[0xE] = true; break;

      case SDLK_z: emulated_system->keypad[0xA] = true; break;
      case SDLK_x: emulated_system->keypad[0x0] = true; break;
      case SDLK_c: emulated_system->keypad[0xB] = true; break;
      case SDLK_v: emulated_system->keypad[0xF] = true; break;

      default: break;
  }
}

static void emulator_user_interface_handle_keyboard_event_key_up(struct EmulatedSystem *emulated_system, SDL_Keycode key) {
  switch (key) {
      // qwerty to CHIP8 keypad
      case SDLK_1: emulated_system->keypad[0x1] = false; break;
      case SDLK_2: emulated_system->keypad[0x2] = false; break;
      case SDLK_3: emulated_system->keypad[0x3] = false; break;
      case SDLK_4: emulated_system->keypad[0xC] = false; break;

      case SDLK_q: emulated_system->keypad[0x4] = false; break;
      case SDLK_w: emulated_system->keypad[0x5] = false; break;
      case SDLK_e: emulated_system->keypad[0x6] = false; break;
      case SDLK_r: emulated_system->keypad[0xD] = false; break;

      case SDLK_a: emulated_system->keypad[0x7] = false; break;
      case SDLK_s: emulated_system->keypad[0x8] = false; break;
      case SDLK_d: emulated_system->keypad[0x9] = false; break;
      case SDLK_f: emulated_system->keypad[0xE] = false; break;

      case SDLK_z: emulated_system->keypad[0xA] = false; break;
      case SDLK_x: emulated_system->keypad[0x0] = false; break;
      case SDLK_c: emulated_system->keypad[0xB] = false; break;
      case SDLK_v: emulated_system->keypad[0xF] = false; break;

      default: break;
  }
}

void emulator_user_interface_update(struct UserInterface *user_interface, struct EmulatedSystem *emulated_system) {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
      switch (event.type) {
          case SDL_QUIT:
              // Exit window; End program
              emulated_system->state = QUIT; // Will exit main emulator loop
              break;

          case SDL_KEYDOWN:
              emulator_user_interface_handle_keyboard_event_key_down(user_interface, emulated_system, event.key.keysym.sym);
              break;

          case SDL_KEYUP:
              emulator_user_interface_handle_keyboard_event_key_up(emulated_system, event.key.keysym.sym);
              break;
      }
  }

  switch (emulated_system->state) {
    case RUNNING:
      emulated_user_interface_draw(user_interface, emulated_system);
      SDL_PauseAudioDevice(user_interface->dev, !user_interface->should_play_sound); // Maybe pause sound
      break;
    case PAUSE:
      pause_menu_user_interface_draw(user_interface);
      break;
    case QUIT:
      break;
  }
}