#include <stdio.h>

#include "user_interface/instruction_print.h"

// TODO: option to choose between cogwods format and our own format.
void instruction_decoded_print(struct DecodedInstruction decoded_instruction) {
    bool found_mnemonic = false;
    int i = 0;
    while (!instruction_mnemonics[i].type == INVALID) {
        if (decoded_instruction.type == instruction_mnemonics[i].type) {
            printf("%s ", instruction_mnemonics[i].name);
            found_mnemonic = true;
        }
        i++;
    }

    if (!found_mnemonic) {
        printf("Unknown\n");
        return;
    }

    switch (decoded_instruction.operands_layout) {
        case ADDRESS:
            if (decoded_instruction.type == ADDRESS_TO_REGISTER_I) printf("I, ");
            printf("0x%03x\n", decoded_instruction.address);
            break;
        case REGISTER_AND_VALUE:
            printf(
                "V%d, %d\n",
                decoded_instruction.register_index,
                decoded_instruction.value
            );
            break;
        case REGISTERS_AND_HALF_VALUE:
            printf(
                "V%d, V%d, %d\n",
                decoded_instruction.register_indexes[0],
                decoded_instruction.register_indexes[1],
                decoded_instruction.half_value
            );
            break;
        case NONE:
        default:
            printf("No operands\n");
            break;
    }
}
