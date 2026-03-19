static void emulated_system_emulate_misc(struct EmulatedSystem *emulated_system) {
    switch (emulated_system->decoded_instruction.value) {
        case 0x0A: {
            // 0xFX0A: Wait for a key press, store the value of the key in VX.
            bool any_key_pressed = false;
            
            for (uint8_t i = 0; i < sizeof(emulated_system->keypad); i++) {
                if (emulated_system->keypad[i]) {
                    emulated_system->V[emulated_system->decoded_instruction.register_index] = i;
                    any_key_pressed = true;
                    break;
                }
            }

            // If no key is pressed, decrement PC to repeat the instruction indefinitely
            if (!any_key_pressed) {
                emulated_system->PC -= 2; 
            }
            // Note: This implements "wait for press". If strict "wait for release" 
            // is required, a 'waiting_for_key' boolean must be added to EmulatedSystem.
            break;
        }

        case 0x1E:
            // 0xFX1E: I += VX; poe VX para reg I.
            emulated_system->I += emulated_system->V[emulated_system->decoded_instruction.register_index];
            break;

        case 0x07:
            // 0xFX07: VX = delay timer
            emulated_system->V[emulated_system->decoded_instruction.register_index] = emulated_system->delay_timer;
            break;

        case 0x15:
            // 0xFX15: delay timer = VX 
            emulated_system->delay_timer = emulated_system->V[emulated_system->decoded_instruction.register_index];
            break;

        case 0x18:
            // 0xFX18: sound timer = VX 
            emulated_system->sound_timer = emulated_system->V[emulated_system->decoded_instruction.register_index];
            break;

        case 0x29:
            // 0xFX29: Set register I to sprite location in memory for character in VX (0x0-0xF)
            emulated_system->I = emulated_system->V[emulated_system->decoded_instruction.register_index] * 5;
            break;

        case 0x33: {
            uint8_t bcd = emulated_system->V[emulated_system->decoded_instruction.register_index]; 
            emulated_system->ram[emulated_system->I+2] = bcd % 10;
            bcd /= 10;
            emulated_system->ram[emulated_system->I+1] = bcd % 10;
            bcd /= 10;
            emulated_system->ram[emulated_system->I] = bcd;
            break;
        }

        case 0x55:
            for (uint8_t i = 0; i <= emulated_system->decoded_instruction.register_index; i++) {
                if (emulated_system->I >= sizeof(emulated_system->ram)) break; // Prevent UB
                emulated_system->ram[emulated_system->I++] = emulated_system->V[i];
            }
            break;

        case 0x65:
            // 0xFX65: Register load V0-VX inclusive from memory offset from I;
            for (uint8_t i = 0; i <= emulated_system->decoded_instruction.register_index; i++) {
                if (emulated_system->extension == CHIP8) 
                    emulated_system->V[i] = emulated_system->ram[emulated_system->I++]; // Incremento de reg I
                else
                    emulated_system->V[i] = emulated_system->ram[emulated_system->I + i];
            }
            break;

        default:
            break;
    }
}