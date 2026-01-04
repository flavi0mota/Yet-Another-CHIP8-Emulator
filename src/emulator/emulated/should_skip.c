static inline bool emulated_system_should_skip_by_key_pressed(struct EmulatedSystem *emulated_system) {
    return emulated_system->keypad[emulated_system->V[emulated_system->decoded_instruction.register_index]];
}

static bool emulated_system_should_skip_by_value(struct EmulatedSystem *emulated_system) {
    uint8_t operands[2];
    switch (emulated_system->decoded_instruction.operands_layout) {
        case REGISTER_AND_VALUE:
            operands[0] = emulated_system->V[emulated_system->decoded_instruction.register_index];
            operands[1] = emulated_system->decoded_instruction.value;
            break;
        case REGISTERS_AND_HALF_VALUE:
            operands[0] = emulated_system->V[emulated_system->decoded_instruction.register_indexes[0]];
            operands[1] = emulated_system->V[emulated_system->decoded_instruction.register_indexes[1]];
            break;
        case ADDRESS:
        case NONE:
        default:
            exit(EXIT_FAILURE);
            break;
    }

    return (
        (emulated_system->decoded_instruction.type == IF_EQUAL_THEN_SKIP && operands[0] == operands[1])
        || (emulated_system->decoded_instruction.type == IF_NOT_EQUAL_THEN_SKIP && operands[0] != operands[1])
    );
}