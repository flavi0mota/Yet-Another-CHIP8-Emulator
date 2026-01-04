#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include "emulated.h"

// draw.c
static void emulated_system_emulate_draw(struct EmulatedSystem *emulated_system);

// misc.c
static void emulated_system_emulate_misc(struct EmulatedSystem *emulated_system);

// should_skip.c
static inline bool emulated_system_should_skip_by_key_pressed(struct EmulatedSystem *emulated_system);
static bool emulated_system_should_skip_by_value(struct EmulatedSystem *emulated_system);

#include "draw.c"
#include "misc.c"
#include "should_skip.c"

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

void emulated_system_initialize(struct EmulatedSystem *emulated_system) {
    memset(emulated_system, 0, sizeof(struct EmulatedSystem)); // clean start
    memcpy(emulated_system->ram, &emulated_system_font, sizeof(emulated_system_font)); // Load font

    *emulated_system = (struct EmulatedSystem){
        .state = RUNNING,
        .PC = emulated_system_entry_point,
        .stack_ptr = emulated_system->stack,
        .extension = CHIP8,
        .instructions_per_frame = 10,
        .frames_per_second = 60
    };
}

// Reads and store encoded 16-bit instruction and decodes it
bool emulated_system_consume_instruction(struct EmulatedSystem *emulated_system) {
    // Get 16-bit instruction from current location PC points to
    emulated_system->encoded_instruction = (
        (emulated_system->ram[emulated_system->PC] << 8) // First byte
        | emulated_system->ram[emulated_system->PC+1] // Second byte
    );

    emulated_system->decoded_instruction = decoded_instruction_from_encoded_instruction(emulated_system->encoded_instruction);

    emulated_system->PC += 2; // Point to next instruction

    if (emulated_system->PC >= 4095) {
        fprintf(stderr, "PC fora do limite: %04X\n", emulated_system->PC);
        emulated_system->state = QUIT;
        return false;
    }
    return true;
}

void emulated_system_emulate_decoded_instruction(struct EmulatedSystem *emulated_system) {
    struct DecodedInstruction *decoded_instruction = &emulated_system->decoded_instruction;


    // Maybe transform register indexes into pointers to the registers
    uint8_t *register_pointers[2];
    uint8_t *register_pointer;

    switch (decoded_instruction->operands_layout) {
        case REGISTER_AND_VALUE:
            register_pointer = &emulated_system->V[decoded_instruction->register_index];
            break;
        case REGISTERS_AND_HALF_VALUE:
            register_pointers[0] = &emulated_system->V[decoded_instruction->register_indexes[0]];
            register_pointers[1] = &emulated_system->V[decoded_instruction->register_indexes[1]];
            break;
        default:
        case ADDRESS:
        case NONE:
            break;
    }

    uint8_t *carry = &emulated_system->V[0xF];

    switch (decoded_instruction->type) {
        case CLEAR:
            memset(emulated_system->display, false, sizeof(emulated_system->display));
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
        case IF_EQUAL_THEN_SKIP:
        case IF_NOT_EQUAL_THEN_SKIP:
            if (emulated_system_should_skip_by_value(emulated_system)) emulated_system->PC += 2;
            break;
        case VALUE_TO_REGISTER:
            *register_pointer = decoded_instruction->value;
            break;
        case SUM_REGISTER:
            *register_pointer += decoded_instruction->value;
            break;
        case REGISTER_TO_REGISTER:
            *register_pointers[0] = *register_pointers[1];
            break;
        case OR_REGISTERS:
            *register_pointers[0] |= *register_pointers[1];
            if (emulated_system->extension == CHIP8) *carry = 0;
            break;
        case AND_REGISTERS:
            *register_pointers[0] &= *register_pointers[1];
            if (emulated_system->extension == CHIP8) *carry = 0;
            break;
        case XOR_REGISTERS:
            *register_pointers[0] ^= *register_pointers[1];
            if (emulated_system->extension == CHIP8) *carry = 0;
            break;
        case SUM_REGISTERS:
            *carry = ((uint16_t)(*register_pointers[0] + *register_pointers[0]) > 255);
            *register_pointers[0] += *register_pointers[1];
            break;
        case SUBTRACT_REGISTERS: 
            *carry = (*register_pointers[1] <= *register_pointers[0]);
            *register_pointers[0] -= *register_pointers[1];
            break;
        case SHIFT_RIGHT_REGISTER:
            if (emulated_system->extension == CHIP8) {
                *carry = *register_pointers[1] & 1; // Use VY
                *register_pointers[0] = *register_pointers[1] >> 1; // Set VX = VY result
            } else {
                *carry = *register_pointers[0] & 1;    // Use VX
                *register_pointers[0] >>= 1;          // Use VX
            }
            break;
        case INVERT_SUBTRACT_REGISTERS:
            *carry = (*register_pointers[0] <= *register_pointers[1]);
            *register_pointers[0] = *register_pointers[1] - *register_pointers[0];
            break;
        case SHIFT_LEFT_REGISTER:
            if (emulated_system->extension == CHIP8) { 
                *carry = (*register_pointers[1] & 0x80) >> 7; // Use VY
                *register_pointers[0] = *register_pointers[1] << 1; // Set VX = VY result
            } else {
                *carry = (*register_pointers[0] & 0x80) >> 7; // VX
                *register_pointers[0] <<= 1; // Use VX
            }
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
        case IF_PRESSED_THEN_SKIP:
            if (emulated_system_should_skip_by_key_pressed(emulated_system)) emulated_system->PC += 2;
            break;
        case IF_NOT_PRESSED_THEN_SKIP:
            if (!emulated_system_should_skip_by_key_pressed(emulated_system)) emulated_system->PC += 2;
            break;
        case MISC:
            emulated_system_emulate_misc(emulated_system);
            break;
        case INVALID:
        default:
            emulated_system->state = QUIT;
            fprintf(stderr, "Instrução inválida: %04X\n", emulated_system->encoded_instruction);
            return;
    }
}
