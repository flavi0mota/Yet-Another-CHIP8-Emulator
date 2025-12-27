// Emulated system (chip8)

#pragma once

#include <stdint.h>

#include "instruction.h"

#define STACK_SIZE 12

// From emulated.c
extern const uint32_t emulated_system_entry_point;
extern const uint8_t emulated_system_font[16][5];

struct EmulatedSystem {
  enum {
    QUIT,
    RUNNING,
    PAUSE,
  } state;
  enum {
    CHIP8,
    SUPERCHIP,
    XOCHIP,
  } extension;
  uint8_t ram[4096]; // 4 kilobytes of fully writable RAM
  bool display[64*32]; // 64x32 pixels, each can be on or off (boolean)
  uint16_t stack[STACK_SIZE]; // stores 16-bit adresses, used for function call and return
  uint16_t *stack_ptr;
  uint8_t V[16]; // general-purpose registers
  uint16_t I; // points at some location in memory
  uint16_t PC; // points at the current instruction in memory
  uint8_t delay_timer; // decrements at the rate of 60hz (60 times per second until reaches 0)
  uint8_t sound_timer; // like the delay_timer
  bool keypad[16];
  const char *rom_name;

  // data as it appears in the rom
  //
  // most significant byte first, the lowest 4 bits form the opcode
  //
  // then may follow either:
  //
  //  - 12-bit memory address
  //
  //  - 4bit-value and 8-bit value.
  //
  //  - 3 parts, 4-bit value each
  uint16_t encoded_instruction; 

  struct DecodedInstruction decoded_instruction;
};

// Writes struct Emulator->EmulatedSystem data to a binary file
bool emulated_save_state(struct EmulatedSystem *emulated_system, const char *filename);

// Loads data from a binary file to Emulator->EmulatedSystem
bool emulated_load_state(struct EmulatedSystem *emulated_system, const char *filename);

void emulated_system_emulate_instruction(struct EmulatedSystem *emulated_system);
void emulated_system_emulate_draw(struct EmulatedSystem *emulated_system);
void emulated_system_emulate_bitwise_arithmetic(struct EmulatedSystem *emulated_system);
void emulated_system_emulate_misc(struct EmulatedSystem *emulated_system);
