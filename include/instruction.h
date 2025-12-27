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

  // Depends on the 12 lower bits from the 16-bit instructiom
  union {
    uint16_t address;

    // Something involving register and value
    struct {
      uint8_t register_index; // 4-bit
      uint8_t value; // 8-bit
    };

    // Arithmetic/bitwise operation between registers
    struct {
      uint8_t register_indexes[2]; // 2x 4-bit (8-bit)
      uint8_t half_value; // 4-bit
    };
  };
};

struct DecodedInstruction decoded_intruction_from_encoded_instruction(uint16_t encoded_instruction);
