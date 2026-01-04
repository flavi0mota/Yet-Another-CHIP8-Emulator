static void emulated_system_emulate_bitwise_arithmetic(struct EmulatedSystem *emulated_system) {
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