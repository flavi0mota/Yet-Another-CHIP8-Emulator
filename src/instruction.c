#include "instruction.h"

struct DecodedInstruction decoded_intruction_from_encoded_instruction(uint16_t encoded_instruction) {
    struct DecodedInstruction decoded_instruction = {0};
    bool involves_address = false;
    bool involves_register_and_value = false;
    bool involves_registers = false;

    switch ((encoded_instruction & 0xF000) >> 12) {
        case 0x0:
            switch (encoded_instruction & 0x00FF) {
                case 0xE0: decoded_instruction.type = CLEAR; break;
                case 0xEE: decoded_instruction.type = RETURN; break;
            }
            break;
        case 0x1: decoded_instruction.type = JUMP; involves_address = true; break;
        case 0x2: decoded_instruction.type = SUBROUTINE; involves_address = true; break;
        case 0x3: decoded_instruction.type = IF_EQUAL_TO_VALUE_THEN_SKIP; involves_register_and_value = true; break;
        case 0x4:
            decoded_instruction.type = IF_NOT_EQUAL_TO_VALUE_THEN_SKIP; involves_register_and_value = true; break;
        case 0x5:
            if ((encoded_instruction & 0x00F0) != 0) decoded_instruction.type = INVALID;
            else {
                decoded_instruction.type = IF_EQUAL_REGISTERS_THEN_SKIP;
                involves_registers = true;
            }
            break;
        case 0x6: decoded_instruction.type = VALUE_TO_REGISTER; involves_register_and_value = true; break;
        case 0x7: decoded_instruction.type = SUM_REGISTER; involves_register_and_value = true; break;
        case 0x8: decoded_instruction.type = REGISTER_BITWISE_AND_ARITHMETIC; involves_registers = true; break;
        case 0x9: decoded_instruction.type = IF_EQUAL_REGISTERS_THEN_SKIP; involves_registers = true; break;
        case 0xA: decoded_instruction.type = ADDRESS_TO_REGISTER_I; involves_address = true; break;
        case 0xB: decoded_instruction.type = JUMP_WITH_OFFSET; involves_address = true; break;
        case 0xC: decoded_instruction.type = RANDOM_NUMBER_TO_REGISTER; involves_register_and_value = true; break;
        case 0xD: decoded_instruction.type = DRAW; involves_registers = true; break;
        case 0xE: decoded_instruction.type = SKIP_BY_KEY_STATE; involves_register_and_value = true; break;
        case 0xF: decoded_instruction.type = MISC; involves_register_and_value = true; break;
        default: decoded_instruction.type = INVALID;
    }

    if (involves_address) {
        decoded_instruction.address = encoded_instruction & 0x0FFF;
    }
    else if (involves_register_and_value) {
        decoded_instruction.register_index = (encoded_instruction & 0x0F00) >> 8;
        decoded_instruction.value = encoded_instruction & 0x00FF;
    }
    else if (involves_registers) {
        decoded_instruction.register_indexes[0] = (encoded_instruction & 0x0F00) >> 8;
        decoded_instruction.register_indexes[1] = (encoded_instruction & 0x00F0) >> 4;
        decoded_instruction.half_value = (encoded_instruction & 0x000F);
    }

    return decoded_instruction;
}
