// Emulator

#include "emulator.h"

#include <stdbool.h>

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
    memset(&emulator->emulated_system, 0, sizeof(struct EmulatedSystem)); // clean start
    memcpy(&emulator->emulated_system.ram, emulated_system_font, sizeof(emulated_system_font)); // Load font

    // Set defaults
    emulator->emulated_system.state = RUNNING;
    emulator->emulated_system.PC = emulated_system_entry_point;
    emulator->emulated_system.stack_ptr = &emulator->emulated_system.stack[0];
    emulator->instructions_per_second = 600;
    emulator->extension = CHIP8;

    emulator_user_interface_initialize(&emulator->user_interface, &emulator->emulated_system);
    emulator_user_interface_clear_screen(&emulator->user_interface);

    return true;
}

void emulator_update(struct Emulator *emulator) {
    static const float frame_duration = 1000.0f / 60;
    uint64_t remaining_instructions = emulator->instructions_per_second / 60;

    emulator->user_interface.expected_moment_to_draw = SDL_GetTicks64() + frame_duration;

    // Instruction cycle (many of these occur each second)
    while (remaining_instructions > 0) {
        remaining_instructions--;
        emulator_emulate_instruction(emulator);
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

    // Update user interface
    emulator_user_interface_update(&emulator->user_interface, &emulator->emulated_system);
}

bool emulator_emulate_instruction(struct Emulator *emulator) {
    struct EmulatedSystem *emulated_system = &emulator->emulated_system;
    struct EmulatedSystem *chip8 = emulated_system; // TODO: remove this
    bool should_draw = false;
    bool carry; // valor carry flag/VF

    // Get next opcode from ram 
    emulated_system->instruction.opcode = (emulated_system->ram[emulated_system->PC] << 8) | emulated_system->ram[emulated_system->PC+1];

    emulated_system->PC += 2;

    emulated_system->instruction.NNN = emulated_system->instruction.opcode & 0x0FFF;
    emulated_system->instruction.NN = emulated_system->instruction.opcode & 0x0FF;
    emulated_system->instruction.N = emulated_system->instruction.opcode & 0x0F;
    emulated_system->instruction.X = (emulated_system->instruction.opcode >> 8) & 0x0F;
    emulated_system->instruction.Y = (emulated_system->instruction.opcode >> 4) & 0x0F;

    if (emulated_system->PC >= 4095) {
        SDL_Log("PC fora do limite: %04X\n", emulated_system->PC);
        emulated_system->state = QUIT;
        return should_draw;
    }

    // Emulate opcode
    switch ((emulated_system->instruction.opcode >> 12) & 0x0F) {
        case 0x00:
            if (emulated_system->instruction.NN == 0xE0) {
                // 0x00E0: Clear
                memset(&emulated_system->display[0], false, sizeof emulated_system->display);
                should_draw = true;
            } else if (emulated_system->instruction.NN == 0xEE) {
                // 0x00EE: Retorna de subrotina
                emulated_system->PC = *--emulated_system->stack_ptr;

            }
            break;

        case 0x01:
            // 0x1NNN: Pulo para NNN
            emulated_system->PC = emulated_system->instruction.NNN;
            break;

        case 0x02:
            // 0x2NNN: subrotina em NNN
            *emulated_system->stack_ptr++ = emulated_system->PC;  
            emulated_system->PC = emulated_system->instruction.NNN;
            break;

        case 0x03:
            // 0x3XNN: Check if VX == NN, if so, skip the next instruction
            if (emulated_system->V[emulated_system->instruction.X] == emulated_system->instruction.NN)
                emulated_system->PC += 2;
            break;

        case 0x04:
            // 0x4XNN: Check if VX != NN, if so, skip the next instruction
            if (emulated_system->V[emulated_system->instruction.X] != emulated_system->instruction.NN)
                emulated_system->PC += 2;       // Skip next opcode/instruction
            break;

        case 0x05:
            // 0x5XY0: Check if VX == VY, if so, skip the next instruction
            if (emulated_system->instruction.N != 0) break; // Wrong opcode

            if (emulated_system->V[emulated_system->instruction.X] == emulated_system->V[emulated_system->instruction.Y])
                emulated_system->PC += 2;       // Skip next opcode/instruction
            
            break;

        case 0x06:
            // 0x6XNN: Set register VX to NN
            emulated_system->V[emulated_system->instruction.X] = emulated_system->instruction.NN;
            break;

        case 0x07:
            // 0x7XNN: Set register VX += NN
            emulated_system->V[emulated_system->instruction.X] += emulated_system->instruction.NN;
            break;

        case 0x08:
            switch(emulated_system->instruction.N) {
                case 0:
                    // 0x8XY0: Set register VX = VY
                    emulated_system->V[emulated_system->instruction.X] = emulated_system->V[emulated_system->instruction.Y];
                    break;

                case 1:
                    // 0x8XY1: Set register VX |= VY
                    emulated_system->V[emulated_system->instruction.X] |= emulated_system->V[emulated_system->instruction.Y];
                    if (emulator->extension == CHIP8)
                        emulated_system->V[0xF] = 0;  // Reset VF to 0
                    break;

                case 2:
                    // 0x8XY2: Set register VX &= VY
                    emulated_system->V[emulated_system->instruction.X] &= emulated_system->V[emulated_system->instruction.Y];
                    if (emulator->extension == CHIP8)
                        emulated_system->V[0xF] = 0;  // Reset VF to 0
                    break;

                case 3:
                    // 0x8XY3: Set register VX ^= VY
                    emulated_system->V[emulated_system->instruction.X] ^= emulated_system->V[emulated_system->instruction.Y];
                    if (emulator->extension == CHIP8)
                        emulated_system->V[0xF] = 0;  // Reset VF to 0
                    break;

                case 4:
                    // 0x8XY4: Set register VX += VY, set VF to 1 if carry, 0 if not 
                    carry = ((uint16_t)(emulated_system->V[emulated_system->instruction.X] + emulated_system->V[emulated_system->instruction.Y]) > 255);

                    emulated_system->V[emulated_system->instruction.X] += emulated_system->V[emulated_system->instruction.Y];
                    emulated_system->V[0xF] = carry; 
                    break;

                case 5: 
                    // 0x8XY5: Set register VX -= VY, set VF to 1 if there is not a borrow (result is positive/0)
                    carry = (emulated_system->V[emulated_system->instruction.Y] <= emulated_system->V[emulated_system->instruction.X]);

                    emulated_system->V[emulated_system->instruction.X] -= emulated_system->V[emulated_system->instruction.Y];
                    emulated_system->V[0xF] = carry;
                    break;

                case 6:
                    // 0x8XY6: Set register VX >>= 1, store shifted off bit in VF
                    if (emulator->extension == CHIP8) {
                        carry = emulated_system->V[emulated_system->instruction.Y] & 1;    // Use VY
                        emulated_system->V[emulated_system->instruction.X] = emulated_system->V[emulated_system->instruction.Y] >> 1; // Set VX = VY result
                    } else {
                        carry = emulated_system->V[emulated_system->instruction.X] & 1;    // Use VX
                        emulated_system->V[emulated_system->instruction.X] >>= 1;          // Use VX
                    }

                    emulated_system->V[0xF] = carry;
                    break;

                case 7:
                    // 0x8XY7: Set register VX = VY - VX, set VF to 1 if there is not a borrow (result is positive/0)
                    carry = (emulated_system->V[emulated_system->instruction.X] <= emulated_system->V[chip8->instruction.Y]);

                    chip8->V[chip8->instruction.X] = chip8->V[chip8->instruction.Y] - chip8->V[chip8->instruction.X];
                    chip8->V[0xF] = carry;
                    break;

                case 0xE:
                    if (emulator->extension == CHIP8) { 
                        carry = (chip8->V[chip8->instruction.Y] & 0x80) >> 7; // Use VY
                        chip8->V[chip8->instruction.X] = chip8->V[chip8->instruction.Y] << 1; // Set VX = VY result
                    } else {
                        carry = (chip8->V[chip8->instruction.X] & 0x80) >> 7;  // VX
                        chip8->V[chip8->instruction.X] <<= 1;                  // Use VX
                    }

                    chip8->V[0xF] = carry;
                    break;

                default:
                    // Opcode errado ou não existe
                    break;
            }
            break;

        case 0x09:
            // 0x9XY0: Check if VX != VY; Skip next instruction if so
            if (chip8->V[chip8->instruction.X] != chip8->V[chip8->instruction.Y])
                chip8->PC += 2;
            break;

        case 0x0A:
            // 0xANNN: Set index register I to NNN
            chip8->I = chip8->instruction.NNN;
            break;

        case 0x0B:
            // 0xBNNN: Jump to V0 + NNN
            chip8->PC = chip8->V[0] + chip8->instruction.NNN;
            break;

        case 0x0C:
            // 0xCXNN: VX = rand() % 256 & NN (bitwise AND)
            chip8->V[chip8->instruction.X] = (rand() % 256) & chip8->instruction.NN;
            break;

        case 0x0D: {
            // 0xDXYN: Draw N-height sprite at coords X,Y; Read from memory location I;
            //   Screen pixels are XOR'd with sprite bits, 
            //   VF (Carry flag) is set if any screen pixels are set off; This is useful
            //   for collision detection or other reasons.
            uint8_t X_coord = chip8->V[chip8->instruction.X] % emulator->user_interface.desired_window_width;
            uint8_t Y_coord = chip8->V[chip8->instruction.Y] % emulator->user_interface.desired_window_height;
            const uint8_t orig_X = X_coord; // Original X value

            chip8->V[0xF] = 0;  // Initialize carry flag to 0

            // Loop over all N rows of the sprite
            for (uint8_t i = 0; i < chip8->instruction.N; i++) {
                // Get next byte/row of sprite data
                const uint8_t sprite_data = chip8->ram[chip8->I + i];
                X_coord = orig_X;   // Reset X for next row to draw

                for (int8_t j = 7; j >= 0; j--) {
                    // set carry flag
                    bool *pixel = &chip8->display[Y_coord * emulator->user_interface.desired_window_width + X_coord]; 
                    const bool sprite_bit = (sprite_data & (1 << j));

                    if (sprite_bit && *pixel) {
                        chip8->V[0xF] = 1;  
                    }

                    // XOR display pixel
                    *pixel ^= sprite_bit;

                    // Para de desenhar se bater no canto da tela
                    if (++X_coord >= emulator->user_interface.desired_window_width) break;
                }

                if (++Y_coord >= emulator->user_interface.desired_window_height) break;
            }
            should_draw = true; // atualiza tela no próximo tick 60hz
            break;
        }

        case 0x0E:
            if (chip8->instruction.NN == 0x9E) {
                // 0xEX9E: Skip next instruction if key in VX is pressed
                if (chip8->keypad[chip8->V[chip8->instruction.X]])
                    chip8->PC += 2;

            } else if (chip8->instruction.NN == 0xA1) {
                // 0xEX9E: Skip next instruction if key in VX is not pressed
                if (!chip8->keypad[chip8->V[chip8->instruction.X]])
                    chip8->PC += 2;
            }
            break;

        case 0x0F:
            switch (chip8->instruction.NN) {
                case 0x0A: {
                    // 0xFX0A: VX = get_key(); guarda em VX
                    static bool any_key_pressed = false;
                    static uint8_t key = 0xFF;

                    for (uint8_t i = 0; key == 0xFF && i < sizeof chip8->keypad; i++) 
                        if (chip8->keypad[i]) {
                            key = i;   
                            any_key_pressed = true;
                            break;
                        }

                    
                    if (!any_key_pressed) chip8->PC -= 2; 
                    else {
                        // A key has been pressed, also wait until it is released to set the key in VX
                        if (chip8->keypad[key])     // "Busy loop" CHIP8 emulation until key is released
                            chip8->PC -= 2;
                        else {
                            chip8->V[chip8->instruction.X] = key;     // VX = key 
                            key = 0xFF;                        // Reset key não encontrada
                            any_key_pressed = false;
                        }
                    }
                    break;
                }

                case 0x1E:
                    // 0xFX1E: I += VX; poe VX para reg I.
                    chip8->I += chip8->V[chip8->instruction.X];
                    break;

                case 0x07:
                    // 0xFX07: VX = delay timer
                    chip8->V[chip8->instruction.X] = chip8->delay_timer;
                    break;

                case 0x15:
                    // 0xFX15: delay timer = VX 
                    chip8->delay_timer = chip8->V[chip8->instruction.X];
                    break;

                case 0x18:
                    // 0xFX18: sound timer = VX 
                    chip8->sound_timer = chip8->V[chip8->instruction.X];
                    break;

                case 0x29:
                    // 0xFX29: Set register I to sprite location in memory for character in VX (0x0-0xF)
                    chip8->I = chip8->V[chip8->instruction.X] * 5;
                    break;

                case 0x33: {
                    uint8_t bcd = chip8->V[chip8->instruction.X]; 
                    chip8->ram[chip8->I+2] = bcd % 10;
                    bcd /= 10;
                    chip8->ram[chip8->I+1] = bcd % 10;
                    bcd /= 10;
                    chip8->ram[chip8->I] = bcd;
                    break;
                }

                case 0x55:
                    // 0xFX55: Register dump V0-VX inclusive to memory offset from I;
                    for (uint8_t i = 0; i <= chip8->instruction.X; i++)  {
                        if (emulator->extension == CHIP8) 
                            chip8->ram[chip8->I++] = chip8->V[i]; // Incremento de reg I
                        else
                            chip8->ram[chip8->I + i] = chip8->V[i]; 
                    }
                    break;

                case 0x65:
                    // 0xFX65: Register load V0-VX inclusive from memory offset from I;
                    for (uint8_t i = 0; i <= chip8->instruction.X; i++) {
                        if (emulator->extension == CHIP8) 
                            chip8->V[i] = chip8->ram[chip8->I++]; // Incremento de reg I
                        else
                            chip8->V[i] = chip8->ram[chip8->I + i];
                    }
                    break;

                default:
                    break;
            }
            break;
            
        default:
            break;  // Opcode inválido
    }
    return should_draw;
}

void emulator_destroy(struct Emulator *emulator) {
    emulator_user_interface_destroy(&emulator->user_interface);
}