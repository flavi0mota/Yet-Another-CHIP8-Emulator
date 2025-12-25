// Emulator (interpreter)

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "emulated.h"
#include "user_interface/sdl/interface.h"

// From emulated.c
extern const uint32_t emulated_system_entry_point;
extern const uint8_t emulated_system_font;

struct Emulator {
  // how many instructions are executed each second.
  uint32_t instructions_per_second;

  // virtual machine specifications
  enum {
    CHIP8,
    SUPERCHIP,
    XOCHIP,
  } extension;

  // represents the system that will be emulated
  struct EmulatedSystem emulated_system;

  // handles the user interaction with the emulated system (audio, video, keypresses)
  struct UserInterface user_interface;

  const char *rom_name; // binary file loaded into the virtual machine
};

bool emulator_save_state(struct Emulator *emulator, const char *filename);
bool emulator_load_state(struct Emulator *emulator, const char *filename);
bool emulator_load_rom(struct Emulator *emulator, const char* rom_name);
bool emulator_initialize(struct Emulator *emulator);
void emulator_update(struct Emulator *emulator);
bool emulator_emulate_instruction(struct Emulator *emulator);
void emulator_destroy(struct Emulator *emulator);
