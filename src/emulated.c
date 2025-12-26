#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "emulated.h"

const uint32_t emulated_system_entry_point = 0x200; // CHIP8 Roms will be loaded to 0x200
const uint8_t emulated_system_font[] = {
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

bool emulated_save_state(struct EmulatedSystem *emulated_system, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file) return false;
    else if (fwrite(emulated_system, sizeof(struct EmulatedSystem), 1, file) != 1) {
        fclose(file);
        return false;
    }
    else {
        fclose(file);
        return true;
    }
}

bool emulated_load_state(struct EmulatedSystem *emulated_system, const char *filename) {
    FILE *file = fopen(filename, "rb");

    if (!file) {
      fprintf(stderr, "Não foi possível encontrar o save %s\n", filename);
      return false;
    }
    else if (fread(emulated_system, sizeof(struct EmulatedSystem), 1, file) != 1) {
        fprintf(stderr, "Não foi possível ler o save %s\n", filename);
        fclose(file);
        return false;
    }
    else {
        fclose(file);
        return true;
    }
}