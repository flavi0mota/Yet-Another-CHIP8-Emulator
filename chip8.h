#pragma once

#include <stdint.h>

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

bool init_chip8(chip8_t *chip8, const config_t config, const char rom_name[]);
bool save_estado_chip(const chip8_t *chip8, const char *filename);
bool carregar_estado_chip(chip8_t *chip8, const char *filename);
bool set_config_from_args(config_t* config, const int argc, char** argv);
bool init_sdl(sdl_t *sdl, config_t *config);
void clear_screen(const sdl_t sdl, const config_t config);
void final_cleanup(const sdl_t sdl);
void update_screen(const sdl_t *sdl, const config_t *config, chip8_t *chip8);
void update_timers(const sdl_t *sdl, chip8_t *chip8);
void process_frame(chip8_t *chip8, config_t *config, sdl_t *sdl);