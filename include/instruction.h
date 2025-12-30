#pragma once

#include <stdint.h>

// Depends on the 4 higher bits from the 16-bit instructiom
enum DecodedInstructionType {
  INVALID,
  CLEAR,
  RETURN,
  JUMP,
  SUBROUTINE,
  IF_EQUAL_TO_VALUE_THEN_SKIP,
  IF_NOT_EQUAL_TO_VALUE_THEN_SKIP,
  IF_EQUAL_REGISTERS_THEN_SKIP,
  VALUE_TO_REGISTER,
  SUM_REGISTER,
  REGISTER_BITWISE_AND_ARITHMETIC,
  IF_NOT_EQUAL_REGISTERS_THEN_SKIP,
  ADDRESS_TO_REGISTER_I,
  JUMP_WITH_OFFSET,
  RANDOM_NUMBER_TO_REGISTER,
  DRAW,
  SKIP_BY_KEY_STATE,
  MISC,
};

// Convenient representation for each instruction
struct DecodedInstruction {
  enum DecodedInstructionType type;

  // Operands are encoded in 12 lower bits of the 16-bit instruction.
  // These are different layouts of data that can be stored in those 12 bits.
  enum {
    NONE, // No operands (instruction clear for example).
    ADDRESS,
    REGISTER_AND_VALUE,
    REGISTERS_AND_HALF_VALUE,
  } operands_layout;

  // Operands union, based on the operands layout.
  union {
    uint16_t address; // Encoded as 12 bits

    // Register and value
    struct {
      uint8_t register_index; // Encoded as 4 bits
      uint8_t value; // Encoded as 8 bits
    };

    // Registers and half value
    struct {
      uint8_t register_indexes[2]; // 2 4-bit encoded values (8 bits)
      uint8_t half_value; // Encoded as 4 bits
    };
  };
};

struct DecodedInstruction decoded_intruction_from_encoded_instruction(uint16_t encoded_instruction);
uint16_t encoded_instruction_from_decoded_instruction(struct DecodedInstruction decoded_intruction);