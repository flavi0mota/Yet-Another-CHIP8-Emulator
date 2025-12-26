// Emulated system (chip8)

#pragma once

#include <stdint.h>

#define STACK_SIZE 12

// From emulated.c
extern const uint32_t emulated_system_entry_point;
extern const uint8_t emulated_system_font[16][5];

struct Instruction {
  uint16_t opcode; // 1º half-byte
  uint16_t NNN; // 2º, 3º and 4º half-byte (12-bit memory address)
  uint8_t NN; // 3º and 4º half-byte (8-bit number)
  uint8_t N; // 4º half-byte
  uint8_t X; // 2º half-byte
  uint8_t Y; // 3º half-byte
};

struct EmulatedSystem {
  enum {
    QUIT,
    RUNNING,
    PAUSE,
  } state;
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
  struct Instruction instruction;
};

// Writes struct Emulator->EmulatedSystem data to a binary file
bool emulated_save_state(struct EmulatedSystem *emulated_system, const char *filename);

// Loads data from a binary file to Emulator->EmulatedSystem
bool emulated_load_state(struct EmulatedSystem *emulated_system, const char *filename);