// Emulator (interpreter)

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "emulated.h"
#include "user_interface/sdl/interface.h"

struct Emulator {
  unsigned int frames_per_second;

  // how many instructions are executed each frame.
  unsigned int instructions_per_frame;

  // represents the system that will be emulated
  struct EmulatedSystem emulated_system;

  // handles the user interaction with the emulated system (audio, video, keypresses)
  struct UserInterface user_interface;

  const char *rom_name; // binary file loaded into the virtual machine
};

// Loads binary file to emulated system memory
bool emulator_load_rom(struct Emulator *emulator, const char* rom_name);

// Initializes emulator
bool emulator_initialize(struct Emulator *emulator);

// Performs interpretation cycle
void emulator_update(struct Emulator *emulator);

// Destroys struct Emulator
void emulator_destroy(struct Emulator *emulator);
