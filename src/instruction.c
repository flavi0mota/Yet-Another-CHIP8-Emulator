#include <stdbool.h>
#include <stdio.h>

#include "instruction.h"

// TODO: give long and short names to types.
// TODO: decied about instruction overload, example: jp, ld.
const struct InstructionMnemonic instruction_mnemonics[] = {
    {.type = CLEAR, .name = "cls"},
    {.type = RETURN, .name = "ret"},
    {.type = JUMP, .name = "jp"},
    {.type = SUBROUTINE, .name = "call"},
    {.type = IF_EQUAL_THEN_SKIP, .name = "se"},
    {.type = IF_NOT_EQUAL_THEN_SKIP, .name = "sne"},
    {.type = VALUE_TO_REGISTER, .name = "ld"},
    {.type = SUM_REGISTER, .name = "add"},
    {.type = REGISTER_TO_REGISTER, .name = "ld"},
    {.type = OR_REGISTERS, .name = "or"},
    {.type = AND_REGISTERS, .name = "and"},
    {.type = XOR_REGISTERS, .name = "xor"},
    {.type = SUM_REGISTERS, .name = "sum"},
    {.type = SUBTRACT_REGISTERS, .name = "sub"},
    {.type = SHIFT_RIGHT_REGISTER, .name = "shr"},
    {.type = INVERT_SUBTRACT_REGISTERS, .name = "subn"},
    {.type = SHIFT_LEFT_REGISTER, .name = "shl"},
    {.type = ADDRESS_TO_REGISTER_I, .name = "ld"},
    {.type = JUMP_WITH_OFFSET, .name = "jp"},
    {.type = RANDOM_NUMBER_TO_REGISTER, .name = "rnd"},
    {.type = DRAW, .name = "drw"},
    {.type = IF_PRESSED_THEN_SKIP, .name = "skp"},
    {.type = IF_NOT_PRESSED_THEN_SKIP, .name = "sknp"},
    {.type = MISC, .name = "misc"},
    {.type = INVALID}, // Must terminate with this
};

struct DecodedInstruction decoded_instruction_from_string(const char *string) {
    struct DecodedInstruction decoded_instruction = { .type = INVALID };

    // Get type
    char mnemonic[20] = {0};

    if (sscanf(string, "%s\n", mnemonic) == 1) {
        decoded_instruction.operands_layout = NONE;
    }
    else if (sscanf(string, "%s V%u, %u\n", mnemonic, (unsigned int*)&decoded_instruction.register_index, (unsigned int*)&decoded_instruction.value) == 3) {
        decoded_instruction.operands_layout = REGISTER_AND_VALUE;
    }
    else if (sscanf(string, "%s %x\n", mnemonic, (unsigned int*)&decoded_instruction.address) == 2) {
        decoded_instruction.operands_layout = ADDRESS;
    }
    else {
        fprintf(stderr, "Line looks strange O_o\n");
        return decoded_instruction;
    }

    for (int i=0; instruction_mnemonics[i].type != INVALID ; i++) {
        if (strcmp(mnemonic, instruction_mnemonics[i].name) == 0)
            decoded_instruction.type = instruction_mnemonics[i].type;
    }

    // TODO: check for correctness of type and operands

    return decoded_instruction;
}

struct DecodedInstruction decoded_instruction_from_encoded_instruction(uint16_t encoded_instruction) {
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
            decoded_instruction.type = IF_EQUAL_THEN_SKIP;
            decoded_instruction.operands_layout = REGISTER_AND_VALUE;
            break;
        case 0x4:
            decoded_instruction.type = IF_NOT_EQUAL_THEN_SKIP;
            decoded_instruction.operands_layout = REGISTER_AND_VALUE;
            break;
        case 0x5:
            if ((encoded_instruction & 0x000F) != 0) decoded_instruction.type = INVALID;
            else {
                decoded_instruction.type = IF_EQUAL_THEN_SKIP;
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
            decoded_instruction.operands_layout = REGISTERS_AND_HALF_VALUE;
            switch (encoded_instruction & 0x000F) {
                case 0:
                    decoded_instruction.type = REGISTER_TO_REGISTER;
                    break;
                case 1:
                    decoded_instruction.type = OR_REGISTERS;
                    break;
                case 2:
                    decoded_instruction.type = AND_REGISTERS;
                    break;
                case 3:
                    decoded_instruction.type = XOR_REGISTERS;
                    break;
                case 4:
                    decoded_instruction.type = SUM_REGISTERS;
                    break;
                case 5:
                    decoded_instruction.type = SUBTRACT_REGISTERS;
                    break;
                case 6:
                    decoded_instruction.type = SHIFT_RIGHT_REGISTER;
                    break;
                case 7:
                    decoded_instruction.type = INVERT_SUBTRACT_REGISTERS;
                    break;
                case 0xE:
                    decoded_instruction.type = SHIFT_LEFT_REGISTER;
                    break;
                default:
                    decoded_instruction.type = INVALID;
                    decoded_instruction.operands_layout = NONE;
                break;
            }
            break;
        case 0x9:
            decoded_instruction.type = IF_NOT_EQUAL_THEN_SKIP;
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
            switch (encoded_instruction & 0x00FF) {
                case 0x9E: decoded_instruction.type = IF_PRESSED_THEN_SKIP; break;
                case 0xA1: decoded_instruction.type = IF_NOT_PRESSED_THEN_SKIP; break;
                default:
                    decoded_instruction.type = INVALID;
                    decoded_instruction.operands_layout = NONE;
                    return decoded_instruction;
                    break;
            }
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
        case IF_EQUAL_THEN_SKIP:
            switch (decoded_instruction.operands_layout) {
                case REGISTER_AND_VALUE:
                    encoded_instruction |= 0x3000;
                    break;
                case REGISTERS_AND_HALF_VALUE:
                    encoded_instruction |= 0x5000;
                    break;
                default:
                    return 0x0000;
                    break;
            }
            break;
        case IF_NOT_EQUAL_THEN_SKIP:
            switch (decoded_instruction.operands_layout) {
                case REGISTER_AND_VALUE:
                    encoded_instruction |= 0x4000;
                    break;
                case REGISTERS_AND_HALF_VALUE:
                    encoded_instruction |= 0x9000;
                    break;
                default:
                    return 0x0000;
                    break;
            }
            break;
        case VALUE_TO_REGISTER:
            encoded_instruction |= 0x6000;
            break;
        case SUM_REGISTER:
            encoded_instruction |= 0x7000;
            break;
        case REGISTER_TO_REGISTER:
        case OR_REGISTERS:
        case AND_REGISTERS:
        case XOR_REGISTERS:
        case SUM_REGISTERS:
        case SUBTRACT_REGISTERS:
        case SHIFT_RIGHT_REGISTER:
        case INVERT_SUBTRACT_REGISTERS:
        case SHIFT_LEFT_REGISTER:
            encoded_instruction |= 0x8000;
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
        case IF_PRESSED_THEN_SKIP:
        case IF_NOT_PRESSED_THEN_SKIP:
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