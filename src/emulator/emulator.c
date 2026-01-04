// Emulator

#include "emulator.h"

bool emulator_load_rom(struct Emulator *emulator, const char* rom_name) {
    // Open ROM file
    FILE *rom = fopen(rom_name, "rb");
    if (!rom) {
        fprintf(stderr, "Rom file %s is invalid or does not exist\n", rom_name);
        return false;
    }

    // Get/check rom size
    fseek(rom, 0, SEEK_END);
    const size_t rom_size = ftell(rom);
    const size_t max_size = sizeof emulator->emulated_system.ram - emulated_system_entry_point;
    rewind(rom);

    if (rom_size > max_size) {
        fprintf(stderr, "Rom file %s is too big! Rom size: %llu, Max size allowed: %llu\n", 
                rom_name, (long long unsigned)rom_size, (long long unsigned)max_size);
        fclose(rom);
        return false;
    }
    // Load ROM
    else if (fread(&emulator->emulated_system.ram[emulated_system_entry_point], rom_size, 1, rom) != 1) {
        fprintf(stderr, "Could not read Rom file %s into CHIP8 memory\n", 
                rom_name);
        fclose(rom);
        return false;
    }
    else {
        emulator->rom_name = rom_name;
        fclose(rom);
        return true;
    }
    return false;
}

// Cleans emulator, loads font, sets default state
bool emulator_initialize(struct Emulator *emulator) {
    emulated_system_initialize(&emulator->emulated_system);

    emulator->instructions_per_frame = 10;
    emulator->frames_per_second = 60;

    emulator_user_interface_initialize(&emulator->user_interface, &emulator->emulated_system);
    emulator_user_interface_clear_screen(&emulator->user_interface);

    return true;
}

void emulator_update(struct Emulator *emulator) {
    if (emulator->emulated_system.state != PAUSE) {
        const float frame_duration = 1000.0f / emulator->frames_per_second;
        unsigned int remaining_instructions = emulator->instructions_per_frame;

        emulator->user_interface.expected_moment_to_draw = SDL_GetTicks64() + frame_duration;

        // Instruction cycle (many of these occur each second)
        while (remaining_instructions > 0) {
            remaining_instructions--;
            emulated_system_consume_instruction(&emulator->emulated_system);
            emulated_system_emulate_decoded_instruction(&emulator->emulated_system);
        }

        // Update timers
        if (emulator->emulated_system.delay_timer > 0) emulator->emulated_system.delay_timer--;

        if (emulator->emulated_system.sound_timer > 0) {
            emulator->emulated_system.sound_timer--;
            emulator->user_interface.should_play_sound = true;
        }
        else {
            emulator->user_interface.should_play_sound = false;
        }
    }

    // Update user interface
    emulator_user_interface_update(&emulator->user_interface, &emulator->emulated_system);
}

void emulator_destroy(struct Emulator *emulator) {
    emulator_user_interface_destroy(&emulator->user_interface);
}