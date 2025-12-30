#include "instruction.h"

struct DecodedInstruction decoded_intruction_from_encoded_instruction(uint16_t encoded_instruction) {
    struct DecodedInstruction decoded_instruction = {0};

    // Determine type of instruction and layout based on first 4 bits.
    switch ((encoded_instruction & 0xF000) >> 12) {
        case 0x0:
            switch (encoded_instruction & 0x00FF) {
                case 0xE0: decoded_instruction.type = CLEAR; break;
                case 0xEE: decoded_instruction.type = RETURN; break;
            }
            decoded_instruction.operands_layout = NONE;
            break;
        case 0x1:
            decoded_instruction.type = JUMP;
            decoded_instruction.operands_layout = ADDRESS;
            break;
        case 0x2:
            decoded_instruction.type = SUBROUTINE;
            decoded_instruction.operands_layout = ADDRESS;
            break;
        case 0x3:
            decoded_instruction.type = IF_EQUAL_TO_VALUE_THEN_SKIP;
            decoded_instruction.operands_layout = REGISTER_AND_VALUE;
            break;
        case 0x4:
            decoded_instruction.type = IF_NOT_EQUAL_TO_VALUE_THEN_SKIP;
            decoded_instruction.operands_layout = REGISTER_AND_VALUE;
            break;
        case 0x5:
            if ((encoded_instruction & 0x000F) != 0) decoded_instruction.type = INVALID;
            else {
                decoded_instruction.type = IF_EQUAL_REGISTERS_THEN_SKIP;
                decoded_instruction.operands_layout = REGISTERS_AND_HALF_VALUE;
            }
            break;
        case 0x6:
            decoded_instruction.type = VALUE_TO_REGISTER;
            decoded_instruction.operands_layout = REGISTER_AND_VALUE;
            break;
        case 0x7:
            decoded_instruction.type = SUM_REGISTER;
            decoded_instruction.operands_layout = REGISTER_AND_VALUE;
            break;
        case 0x8:
            decoded_instruction.type = REGISTER_BITWISE_AND_ARITHMETIC;
            decoded_instruction.operands_layout = REGISTERS_AND_HALF_VALUE;
            break;
        case 0x9:
            decoded_instruction.type = IF_EQUAL_REGISTERS_THEN_SKIP;
            decoded_instruction.operands_layout = REGISTERS_AND_HALF_VALUE;
            break;
        case 0xA:
            decoded_instruction.type = ADDRESS_TO_REGISTER_I;
            decoded_instruction.operands_layout = ADDRESS;
            break;
        case 0xB:
            decoded_instruction.type = JUMP_WITH_OFFSET;
            decoded_instruction.operands_layout = ADDRESS;
            break;
        case 0xC:
            decoded_instruction.type = RANDOM_NUMBER_TO_REGISTER;
            decoded_instruction.operands_layout = REGISTER_AND_VALUE;
            break;
        case 0xD:
            decoded_instruction.type = DRAW;
            decoded_instruction.operands_layout = REGISTERS_AND_HALF_VALUE;
            break;
        case 0xE:
            decoded_instruction.type = SKIP_BY_KEY_STATE;
            decoded_instruction.operands_layout = REGISTER_AND_VALUE;
            break;
        case 0xF:
            decoded_instruction.type = MISC;
            decoded_instruction.operands_layout = REGISTER_AND_VALUE;
            break;
        default:
            decoded_instruction.type = INVALID;
            decoded_instruction.operands_layout = NONE;
    }

    // Decode operand values according to the layout.
    switch (decoded_instruction.operands_layout) {
        case ADDRESS:
            decoded_instruction.address = encoded_instruction & 0x0FFF;
            break;
        case REGISTER_AND_VALUE:
            decoded_instruction.register_index = (encoded_instruction & 0x0F00) >> 8;
            decoded_instruction.value = encoded_instruction & 0x00FF;
            break;
        case REGISTERS_AND_HALF_VALUE:
            decoded_instruction.register_indexes[0] = (encoded_instruction & 0x0F00) >> 8;
            decoded_instruction.register_indexes[1] = (encoded_instruction & 0x00F0) >> 4;
            decoded_instruction.half_value = (encoded_instruction & 0x000F);
            break;
        case NONE:
        default:
            break;
    }

    return decoded_instruction;
}

uint16_t encoded_instruction_from_decoded_instruction(struct DecodedInstruction decoded_instruction) {
    uint16_t encoded_instruction = 0x0000;

    // Encode the operands (last 12 bits)
    switch (decoded_instruction.operands_layout) {
        case ADDRESS:
            encoded_instruction = decoded_instruction.address;
            break;
        case REGISTER_AND_VALUE:
            encoded_instruction += decoded_instruction.register_index;
            encoded_instruction <<= 4;
            encoded_instruction += decoded_instruction.value;
            encoded_instruction <<= 8;
            break;
        case REGISTERS_AND_HALF_VALUE:
            encoded_instruction += decoded_instruction.register_indexes[0];
            encoded_instruction <<= 4;
            encoded_instruction += decoded_instruction.register_indexes[1];
            encoded_instruction <<= 4;
            encoded_instruction += decoded_instruction.half_value;
            encoded_instruction <<= 8;
            break;
        case NONE:
        default:
            break;
    }

    // Encode type (first 4 bits)
    // OR is used for masking (zeros mean what was there is kept)
    // But on some cases, like clear and return, masking is not used
    switch (decoded_instruction.type) {
        case CLEAR:
            encoded_instruction = 0x00E0;
            break;
        case RETURN:
            encoded_instruction = 0x00EE;
            break;
        case JUMP:
            encoded_instruction |= 0x1000;
            break;
        case SUBROUTINE:
            encoded_instruction |= 0x2000;
            break;
        case IF_EQUAL_TO_VALUE_THEN_SKIP:
            encoded_instruction |= 0x3000;
            break;
        case IF_NOT_EQUAL_TO_VALUE_THEN_SKIP:
            encoded_instruction |= 0x4000;
            break;
        case IF_EQUAL_REGISTERS_THEN_SKIP:
            encoded_instruction |= 0x5000;
            break;
        case VALUE_TO_REGISTER:
            encoded_instruction |= 0x6000;
            break;
        case SUM_REGISTER:
            encoded_instruction |= 0x7000;
            break;
        case REGISTER_BITWISE_AND_ARITHMETIC:
            encoded_instruction |= 0x8000;
            break;
        case IF_NOT_EQUAL_REGISTERS_THEN_SKIP:
            encoded_instruction |= 0x9000;
            break;
        case ADDRESS_TO_REGISTER_I:
            encoded_instruction |= 0xA000;
            break;
        case JUMP_WITH_OFFSET:
            encoded_instruction |= 0xB000;
            break;
        case RANDOM_NUMBER_TO_REGISTER:
            encoded_instruction |= 0xC000;
            break;
        case DRAW:
            encoded_instruction |= 0xD000;
            break;
        case SKIP_BY_KEY_STATE:
            encoded_instruction |= 0xE000;
            break;
        case MISC:
            encoded_instruction |= 0xF000;
            break;
        case INVALID:
        default:
            return 0x0000;
            break;
    }

    return encoded_instruction;
}