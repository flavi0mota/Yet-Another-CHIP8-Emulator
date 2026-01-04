#pragma once

#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "emulated.h"

struct UserInterface {
  uint32_t desired_window_width;
  uint32_t desired_window_height;
  uint32_t fg_color;
  uint32_t bg_color;
  uint32_t scale_factor;
  bool pixel_outlines;
  uint32_t square_wave_freq;
  uint32_t audio_sample_rate;
  int16_t volume;
  float color_lerp_rate;
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_AudioSpec want, have;
  SDL_AudioDeviceID dev;
  uint32_t pixel_color[64*32];
  uint64_t expected_moment_to_draw;
  bool should_play_sound;
  TTF_Font* font;
  struct {
    SDL_Surface* message_surface;
    SDL_Texture* message;
  } pause_menu;
  struct {
    bool is_active;
    SDL_Surface* message_surface;
    SDL_Texture* message;
  } disassembling;
};

// Misc
uint32_t user_interface_color_lerp(const uint32_t start_color, const uint32_t end_color, const float t);

// For emulator
void emulator_user_interface_destroy(struct UserInterface *user_interface);
void emulator_user_interface_clear_screen(struct UserInterface *user_interface);
void emulator_user_interface_audio_callback(void *userdata, uint8_t *stream, int len);
bool emulator_user_interface_initialize(struct UserInterface *user_interface, struct EmulatedSystem *emulated_system);
void emulator_user_interface_update(struct UserInterface *user_interface, struct EmulatedSystem *emulated_system);
bool emulator_user_interface_font_load(struct UserInterface *user_interface, const char* path);
