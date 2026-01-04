#pragma once

#include <stdint.h>
#include <string.h>

// Depends on the 4 higher bits from the 16-bit instruction
enum DecodedInstructionType {
  INVALID,
  CLEAR,
  RETURN,
  JUMP,
  SUBROUTINE,
  IF_EQUAL_THEN_SKIP,
  IF_NOT_EQUAL_THEN_SKIP,
  VALUE_TO_REGISTER,
  SUM_REGISTER,
  REGISTER_BITWISE_AND_ARITHMETIC, // TODO: expand this
  ADDRESS_TO_REGISTER_I,
  JUMP_WITH_OFFSET,
  RANDOM_NUMBER_TO_REGISTER,
  DRAW,
  IF_PRESSED_THEN_SKIP,
  IF_NOT_PRESSED_THEN_SKIP,
  MISC, // TODO: expand this
};

struct InstructionMnemonic {
    enum DecodedInstructionType type;
    const char* const name;
};

extern const struct InstructionMnemonic instruction_mnemonics[];

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

// The string to be parsed into a decoded instructions is consumed until either a newline, a NULL or a EOF after the instruction or the last operand, depending on the instruction type. Thus, it is ok to pass a pointer to a buffer position, given these conditions are met.
// Example of string: add V0, 4\n
struct DecodedInstruction decoded_instruction_from_string(const char *string);

struct DecodedInstruction decoded_instruction_from_encoded_instruction(uint16_t encoded_instruction);
uint16_t encoded_instruction_from_decoded_instruction(struct DecodedInstruction decoded_intruction);