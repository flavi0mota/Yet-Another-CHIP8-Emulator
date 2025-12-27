#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include "emulated.h"

const uint32_t emulated_system_entry_point = 0x200; // CHIP8 Roms will be loaded to 0x200
const uint8_t emulated_system_font[16][5] = {
    {0xF0, 0x90, 0x90, 0x90, 0xF0},   // 0
    {0x20, 0x60, 0x20, 0x20, 0x70},   // 1
    {0xF0, 0x10, 0xF0, 0x80, 0xF0},   // 2
    {0xF0, 0x10, 0xF0, 0x10, 0xF0},   // 3
    {0x90, 0x90, 0xF0, 0x10, 0x10},   // 4
    {0xF0, 0x80, 0xF0, 0x10, 0xF0},   // 5
    {0xF0, 0x80, 0xF0, 0x90, 0xF0},   // 6
    {0xF0, 0x10, 0x20, 0x40, 0x40},   // 7
    {0xF0, 0x90, 0xF0, 0x90, 0xF0},   // 8
    {0xF0, 0x90, 0xF0, 0x10, 0xF0},   // 9
    {0xF0, 0x90, 0xF0, 0x90, 0x90},   // A
    {0xE0, 0x90, 0xE0, 0x90, 0xE0},   // B
    {0xF0, 0x80, 0x80, 0x80, 0xF0},   // C
    {0xE0, 0x90, 0x90, 0x90, 0xE0},   // D
    {0xF0, 0x80, 0xF0, 0x80, 0xF0},   // E
    {0xF0, 0x80, 0xF0, 0x80, 0x80},   // F
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

// Reads and store encoded 16-bit instruction and decodes it
static inline bool emulated_system_consume_instruction(struct EmulatedSystem *emulated_system) {
    // Get 16-bit instruction from current location PC points to
    emulated_system->encoded_instruction = (
        (emulated_system->ram[emulated_system->PC] << 8) // First byte
        | emulated_system->ram[emulated_system->PC+1] // Second byte
    );

    emulated_system->PC += 2; // Point to next instruction

    if (emulated_system->PC >= 4095) {
        fprintf(stderr, "PC fora do limite: %04X\n", emulated_system->PC);
        emulated_system->state = QUIT;
        return false;
    }
    return true;
}

void emulated_system_emulate_instruction(struct EmulatedSystem *emulated_system) {
    struct DecodedInstruction *decoded_instruction = &emulated_system->decoded_instruction;

    // Consume encoded instruction
    emulated_system_consume_instruction(emulated_system);
    // Decode instruction
    *decoded_instruction = decoded_intruction_from_encoded_instruction(emulated_system->encoded_instruction);

    if (emulated_system->decoded_instruction.type == INVALID) {
        fprintf(stderr, "Instrução inválida: %04X\n", emulated_system->encoded_instruction);
        emulated_system->state = QUIT;
        return;
    }

    switch (decoded_instruction->type) {
        case CLEAR:
            memset(&emulated_system->display[0], false, sizeof emulated_system->display);
            break;
        case RETURN:
            emulated_system->PC = *--emulated_system->stack_ptr;
            break;
        case JUMP:
            emulated_system->PC = decoded_instruction->address;
            break;
        case SUBROUTINE:
            *emulated_system->stack_ptr++ = emulated_system->PC;  
            emulated_system->PC = decoded_instruction->address;
            break;
        case IF_EQUAL_TO_VALUE_THEN_SKIP:
            if (emulated_system->V[decoded_instruction->register_index] == decoded_instruction->value)
                emulated_system->PC += 2;
            break;
        case IF_NOT_EQUAL_TO_VALUE_THEN_SKIP:
            if (emulated_system->V[decoded_instruction->register_index] != decoded_instruction->value)
                emulated_system->PC += 2;
            break;
        case IF_EQUAL_REGISTERS_THEN_SKIP:
            if (emulated_system->V[decoded_instruction->register_indexes[0]] == emulated_system->V[decoded_instruction->register_indexes[1]])
                emulated_system->PC += 2;
            break;
        case VALUE_TO_REGISTER:
            emulated_system->V[decoded_instruction->register_index] = decoded_instruction->value;
            break;
        case SUM_REGISTER:
            emulated_system->V[decoded_instruction->register_index] += decoded_instruction->value;
            break;
        case REGISTER_BITWISE_AND_ARITHMETIC:
            emulated_system_emulate_bitwise_arithmetic(emulated_system);
            break;
        case IF_NOT_EQUAL_REGISTERS_THEN_SKIP:
            if (emulated_system->V[decoded_instruction->register_indexes[0]] != emulated_system->V[decoded_instruction->register_indexes[1]])
                emulated_system->PC += 2;
            break;
        case ADDRESS_TO_REGISTER_I:
            emulated_system->I = decoded_instruction->address;
            break;
        case JUMP_WITH_OFFSET:
            emulated_system->PC = emulated_system->V[0] + decoded_instruction->address;
            break;
        case RANDOM_NUMBER_TO_REGISTER:
            // 0xCXNN: VX = rand() % 256 & NN (bitwise AND)
            emulated_system->V[decoded_instruction->register_index] = (rand() % 256) & decoded_instruction->value;
            break;
        case DRAW:
            emulated_system_emulate_draw(emulated_system);
            break;
        case SKIP_BY_KEY_STATE:
            if (decoded_instruction->value == 0x9E) {
                // 0xEX9E: Skip next instruction if key in VX is pressed
                if (emulated_system->keypad[emulated_system->V[decoded_instruction->register_index]])
                    emulated_system->PC += 2;

            } else if (decoded_instruction->value == 0xA1) {
                // 0xEX9E: Skip next instruction if key in VX is not pressed
                if (!emulated_system->keypad[emulated_system->V[decoded_instruction->register_index]])
                    emulated_system->PC += 2;
            }
            break;
        case MISC:
            emulated_system_emulate_misc(emulated_system);
            break;
        case INVALID:
        default:
            break;
    }
}

void emulated_system_emulate_misc(struct EmulatedSystem *emulated_system) {
    switch (emulated_system->decoded_instruction.value) {
        case 0x0A: {
            // 0xFX0A: VX = get_key(); guarda em VX
            static bool any_key_pressed = false;
            static uint8_t key = 0xFF;

            for (uint8_t i = 0; key == 0xFF && i < sizeof emulated_system->keypad; i++) 
                if (emulated_system->keypad[i]) {
                    key = i;   
                    any_key_pressed = true;
                    break;
                }

            if (!any_key_pressed) emulated_system->PC -= 2; 
            else {
                // A key has been pressed, also wait until it is released to set the key in VX
                if (emulated_system->keypad[key])     // "Busy loop" CHIP8 emulation until key is released
                    emulated_system->PC -= 2;
                else {
                    emulated_system->V[emulated_system->decoded_instruction.register_index] = key; // VX = key 
                    key = 0xFF; // Reset key não encontrada
                    any_key_pressed = false;
                }
            }
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
            // 0xFX55: Register dump V0-VX inclusive to memory offset from I;
            for (uint8_t i = 0; i <= emulated_system->decoded_instruction.register_index; i++)  {
                if (emulated_system->extension == CHIP8)
                    emulated_system->ram[emulated_system->I++] = emulated_system->V[i]; // Incremento de reg I
                else
                    emulated_system->ram[emulated_system->I + i] = emulated_system->V[i]; 
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

void emulated_system_emulate_draw(struct EmulatedSystem *emulated_system) {
    // 0xDXYN: Draw N-height sprite at coords X,Y; Read from memory location I;
    //   Screen pixels are XOR'd with sprite bits, 
    //   VF (Carry flag) is set if any screen pixels are set off; This is useful
    //   for collision detection or other reasons.
    uint32_t desired_window_width = 64; // TODO: fix this
    uint32_t desired_window_height = 32;
    uint8_t X_coord = emulated_system->V[emulated_system->decoded_instruction.register_indexes[0]] % desired_window_width;
    uint8_t Y_coord = emulated_system->V[emulated_system->decoded_instruction.register_indexes[1]] % desired_window_height;
    const uint8_t orig_X = X_coord; // Original X value

    emulated_system->V[0xF] = 0;  // Initialize carry flag to 0

    // Loop over all N rows of the sprite
    for (uint8_t i = 0; i < emulated_system->decoded_instruction.half_value; i++) {
        // Get next byte/row of sprite data
        const uint8_t sprite_data = emulated_system->ram[emulated_system->I + i];
        X_coord = orig_X;   // Reset X for next row to draw

        for (int8_t j = 7; j >= 0; j--) {
            // set carry flag
            bool *pixel = &emulated_system->display[Y_coord * desired_window_width + X_coord]; 
            const bool sprite_bit = (sprite_data & (1 << j));

            if (sprite_bit && *pixel) {
                emulated_system->V[0xF] = 1;  
            }

            // XOR display pixel
            *pixel ^= sprite_bit;

            // Para de desenhar se bater no cantos para esquerda ou direita
            if (++X_coord >= desired_window_width) break;
        }

        if (++Y_coord >= desired_window_height) break;
    }
}

void emulated_system_emulate_bitwise_arithmetic(struct EmulatedSystem *emulated_system) {
    uint8_t *emulated_register[2];
    emulated_register[0] = &emulated_system->V[emulated_system->decoded_instruction.register_indexes[0]];
    emulated_register[1] = &emulated_system->V[emulated_system->decoded_instruction.register_indexes[1]];
    bool carry; // valor carry flag/VF

    switch(emulated_system->decoded_instruction.half_value) {
        case 0:
            *emulated_register[0] = *emulated_register[1];
            break;
        case 1:
            *emulated_register[0] |= *emulated_register[1];
            if (emulated_system->extension == CHIP8) emulated_system->V[0xF] = 0;
            break;
        case 2:
            *emulated_register[0] &= *emulated_register[1];
            if (emulated_system->extension == CHIP8) emulated_system->V[0xF] = 0;
            break;
        case 3:
            *emulated_register[0] ^= *emulated_register[1];
            if (emulated_system->extension == CHIP8) emulated_system->V[0xF] = 0;
            break;
        case 4:
            // 0x8XY4: Set register VX += VY, set VF to 1 if carry, 0 if not 
            carry = ((uint16_t)(*emulated_register[0] + *emulated_register[0]) > 255);
            *emulated_register[0] += *emulated_register[1];
            emulated_system->V[0xF] = carry; 
            break;
        case 5: 
            // 0x8XY5: Set register VX -= VY, set VF to 1 if there is not a borrow (result is positive/0)
            carry = (*emulated_register[1] <= *emulated_register[0]);
            *emulated_register[0] -= *emulated_register[1];
            emulated_system->V[0xF] = carry;
            break;
        case 6:
            // 0x8XY6: Set register VX >>= 1, store shifted off bit in VF
            if (emulated_system->extension == CHIP8) {
                carry = *emulated_register[1] & 1; // Use VY
                *emulated_register[0] = *emulated_register[1] >> 1; // Set VX = VY result
            } else {
                carry = *emulated_register[0] & 1;    // Use VX
                *emulated_register[0] >>= 1;          // Use VX
            }
            emulated_system->V[0xF] = carry;
            break;
        case 7:
            // 0x8XY7: Set register VX = VY - VX, set VF to 1 if there is not a borrow (result is positive/0)
            carry = (*emulated_register[0] <= *emulated_register[1]);
            *emulated_register[0] = *emulated_register[1] - *emulated_register[0];
            emulated_system->V[0xF] = carry;
            break;
        case 0xE:
            if (emulated_system->extension == CHIP8) { 
                carry = (*emulated_register[1] & 0x80) >> 7; // Use VY
                *emulated_register[0] = *emulated_register[1] << 1; // Set VX = VY result
            } else {
                carry = (*emulated_register[0] & 0x80) >> 7; // VX
                *emulated_register[0] <<= 1; // Use VX
            }
            emulated_system->V[0xF] = carry;
            break;
        default:
            // Opcode errado ou não existe
            break;
    }
}