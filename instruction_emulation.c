#include <stdbool.h>

#include "instruction_emulation.h"

void emulate_instruction(chip8_t *chip8, const config_t config) {
    bool carry;   // valor carry flag/VF

    // Get next opcode from ram 
    chip8->inst.opcode = (chip8->ram[chip8->PC] << 8) | chip8->ram[chip8->PC+1];
    chip8->PC += 2;

    chip8->inst.NNN = chip8->inst.opcode & 0x0FFF;
    chip8->inst.NN = chip8->inst.opcode & 0x0FF;
    chip8->inst.N = chip8->inst.opcode & 0x0F;
    chip8->inst.X = (chip8->inst.opcode >> 8) & 0x0F;
    chip8->inst.Y = (chip8->inst.opcode >> 4) & 0x0F;

    // Emulate opcode
    switch ((chip8->inst.opcode >> 12) & 0x0F) {
        case 0x00:
            if (chip8->inst.NN == 0xE0) {
                // 0x00E0: Clear
                memset(&chip8->display[0], false, sizeof chip8->display);
                chip8->draw = true;
            } else if (chip8->inst.NN == 0xEE) {
                // 0x00EE: Retorna de subrotina
                chip8->PC = *--chip8->stack_ptr;

            } else {
            }

            break;

        case 0x01:
            // 0x1NNN: Pulo para NNN
            chip8->PC = chip8->inst.NNN;
            break;

        case 0x02:
            // 0x2NNN: subrotina em NNN
            *chip8->stack_ptr++ = chip8->PC;  
            chip8->PC = chip8->inst.NNN;
            break;

        case 0x03:
            // 0x3XNN: Check if VX == NN, if so, skip the next instruction
            if (chip8->V[chip8->inst.X] == chip8->inst.NN)
                chip8->PC += 2;
            break;

        case 0x04:
            // 0x4XNN: Check if VX != NN, if so, skip the next instruction
            if (chip8->V[chip8->inst.X] != chip8->inst.NN)
                chip8->PC += 2;       // Skip next opcode/instruction
            break;

        case 0x05:
            // 0x5XY0: Check if VX == VY, if so, skip the next instruction
            if (chip8->inst.N != 0) break; // Wrong opcode

            if (chip8->V[chip8->inst.X] == chip8->V[chip8->inst.Y])
                chip8->PC += 2;       // Skip next opcode/instruction
            
            break;

        case 0x06:
            // 0x6XNN: Set register VX to NN
            chip8->V[chip8->inst.X] = chip8->inst.NN;
            break;

        case 0x07:
            // 0x7XNN: Set register VX += NN
            chip8->V[chip8->inst.X] += chip8->inst.NN;
            break;

        case 0x08:
            switch(chip8->inst.N) {
                case 0:
                    // 0x8XY0: Set register VX = VY
                    chip8->V[chip8->inst.X] = chip8->V[chip8->inst.Y];
                    break;

                case 1:
                    // 0x8XY1: Set register VX |= VY
                    chip8->V[chip8->inst.X] |= chip8->V[chip8->inst.Y];
                    if (config.current_extension == CHIP8)
                        chip8->V[0xF] = 0;  // Reset VF to 0
                    break;

                case 2:
                    // 0x8XY2: Set register VX &= VY
                    chip8->V[chip8->inst.X] &= chip8->V[chip8->inst.Y];
                    if (config.current_extension == CHIP8)
                        chip8->V[0xF] = 0;  // Reset VF to 0
                    break;

                case 3:
                    // 0x8XY3: Set register VX ^= VY
                    chip8->V[chip8->inst.X] ^= chip8->V[chip8->inst.Y];
                    if (config.current_extension == CHIP8)
                        chip8->V[0xF] = 0;  // Reset VF to 0
                    break;

                case 4:
                    // 0x8XY4: Set register VX += VY, set VF to 1 if carry, 0 if not 
                    carry = ((uint16_t)(chip8->V[chip8->inst.X] + chip8->V[chip8->inst.Y]) > 255);

                    chip8->V[chip8->inst.X] += chip8->V[chip8->inst.Y];
                    chip8->V[0xF] = carry; 
                    break;

                case 5: 
                    // 0x8XY5: Set register VX -= VY, set VF to 1 if there is not a borrow (result is positive/0)
                    carry = (chip8->V[chip8->inst.Y] <= chip8->V[chip8->inst.X]);

                    chip8->V[chip8->inst.X] -= chip8->V[chip8->inst.Y];
                    chip8->V[0xF] = carry;
                    break;

                case 6:
                    // 0x8XY6: Set register VX >>= 1, store shifted off bit in VF
                    if (config.current_extension == CHIP8) {
                        carry = chip8->V[chip8->inst.Y] & 1;    // Use VY
                        chip8->V[chip8->inst.X] = chip8->V[chip8->inst.Y] >> 1; // Set VX = VY result
                    } else {
                        carry = chip8->V[chip8->inst.X] & 1;    // Use VX
                        chip8->V[chip8->inst.X] >>= 1;          // Use VX
                    }

                    chip8->V[0xF] = carry;
                    break;

                case 7:
                    // 0x8XY7: Set register VX = VY - VX, set VF to 1 if there is not a borrow (result is positive/0)
                    carry = (chip8->V[chip8->inst.X] <= chip8->V[chip8->inst.Y]);

                    chip8->V[chip8->inst.X] = chip8->V[chip8->inst.Y] - chip8->V[chip8->inst.X];
                    chip8->V[0xF] = carry;
                    break;

                case 0xE:
                    if (config.current_extension == CHIP8) { 
                        carry = (chip8->V[chip8->inst.Y] & 0x80) >> 7; // Use VY
                        chip8->V[chip8->inst.X] = chip8->V[chip8->inst.Y] << 1; // Set VX = VY result
                    } else {
                        carry = (chip8->V[chip8->inst.X] & 0x80) >> 7;  // VX
                        chip8->V[chip8->inst.X] <<= 1;                  // Use VX
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
            if (chip8->V[chip8->inst.X] != chip8->V[chip8->inst.Y])
                chip8->PC += 2;
            break;

        case 0x0A:
            // 0xANNN: Set index register I to NNN
            chip8->I = chip8->inst.NNN;
            break;

        case 0x0B:
            // 0xBNNN: Jump to V0 + NNN
            chip8->PC = chip8->V[0] + chip8->inst.NNN;
            break;

        case 0x0C:
            // 0xCXNN: VX = rand() % 256 & NN (bitwise AND)
            chip8->V[chip8->inst.X] = (rand() % 256) & chip8->inst.NN;
            break;

        case 0x0D: {
            // 0xDXYN: Draw N-height sprite at coords X,Y; Read from memory location I;
            //   Screen pixels are XOR'd with sprite bits, 
            //   VF (Carry flag) is set if any screen pixels are set off; This is useful
            //   for collision detection or other reasons.
            uint8_t X_coord = chip8->V[chip8->inst.X] % config.window_width;
            uint8_t Y_coord = chip8->V[chip8->inst.Y] % config.window_height;
            const uint8_t orig_X = X_coord; // Original X value

            chip8->V[0xF] = 0;  // Initialize carry flag to 0

            // Loop over all N rows of the sprite
            for (uint8_t i = 0; i < chip8->inst.N; i++) {
                // Get next byte/row of sprite data
                const uint8_t sprite_data = chip8->ram[chip8->I + i];
                X_coord = orig_X;   // Reset X for next row to draw

                for (int8_t j = 7; j >= 0; j--) {
                    // set carry flag
                    bool *pixel = &chip8->display[Y_coord * config.window_width + X_coord]; 
                    const bool sprite_bit = (sprite_data & (1 << j));

                    if (sprite_bit && *pixel) {
                        chip8->V[0xF] = 1;  
                    }

                    // XOR display pixel
                    *pixel ^= sprite_bit;

                    // Para de desenhar se bater no canto da tela
                    if (++X_coord >= config.window_width) break;
                }

                if (++Y_coord >= config.window_height) break;
            }
            chip8->draw = true; // atualiza tela no próximo tick 60hz
            break;
        }

        case 0x0E:
            if (chip8->inst.NN == 0x9E) {
                // 0xEX9E: Skip next instruction if key in VX is pressed
                if (chip8->keypad[chip8->V[chip8->inst.X]])
                    chip8->PC += 2;

            } else if (chip8->inst.NN == 0xA1) {
                // 0xEX9E: Skip next instruction if key in VX is not pressed
                if (!chip8->keypad[chip8->V[chip8->inst.X]])
                    chip8->PC += 2;
            }
            break;

        case 0x0F:
            switch (chip8->inst.NN) {
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
                            chip8->V[chip8->inst.X] = key;     // VX = key 
                            key = 0xFF;                        // Reset key não encontrada
                            any_key_pressed = false;
                        }
                    }
                    break;
                }

                case 0x1E:
                    // 0xFX1E: I += VX; poe VX para reg I.
                    chip8->I += chip8->V[chip8->inst.X];
                    break;

                case 0x07:
                    // 0xFX07: VX = delay timer
                    chip8->V[chip8->inst.X] = chip8->delay_timer;
                    break;

                case 0x15:
                    // 0xFX15: delay timer = VX 
                    chip8->delay_timer = chip8->V[chip8->inst.X];
                    break;

                case 0x18:
                    // 0xFX18: sound timer = VX 
                    chip8->sound_timer = chip8->V[chip8->inst.X];
                    break;

                case 0x29:
                    // 0xFX29: Set register I to sprite location in memory for character in VX (0x0-0xF)
                    chip8->I = chip8->V[chip8->inst.X] * 5;
                    break;

                case 0x33: {
                    uint8_t bcd = chip8->V[chip8->inst.X]; 
                    chip8->ram[chip8->I+2] = bcd % 10;
                    bcd /= 10;
                    chip8->ram[chip8->I+1] = bcd % 10;
                    bcd /= 10;
                    chip8->ram[chip8->I] = bcd;
                    break;
                }

                case 0x55:
                    // 0xFX55: Register dump V0-VX inclusive to memory offset from I;
                    for (uint8_t i = 0; i <= chip8->inst.X; i++)  {
                        if (config.current_extension == CHIP8) 
                            chip8->ram[chip8->I++] = chip8->V[i]; // Incremento de reg I
                        else
                            chip8->ram[chip8->I + i] = chip8->V[i]; 
                    }
                    break;

                case 0x65:
                    // 0xFX65: Register load V0-VX inclusive from memory offset from I;
                    for (uint8_t i = 0; i <= chip8->inst.X; i++) {
                        if (config.current_extension == CHIP8) 
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
}