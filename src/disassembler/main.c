#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "instruction.h"
#include "user_interface/instruction_print.h"

struct Disassembler {
    const char *input_filename;
    const char *output_filename;
};

static bool consume_command_line_arguments(struct Disassembler *disassembler, int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_filename>\n", argv[0]);
        return false;
    }

    disassembler->input_filename = argv[argc-1];
    printf("Opening: %s\n", disassembler->input_filename);

    for (int i = 1; i < argc; i++) {
         if (strncmp(argv[i], "--output", strlen("--output")) == 0) {
             disassembler->output_filename = argv[i++];
         }
    }

    return true;
}

int main(int argc, char **argv) {
    struct Disassembler disassembler = {0};

    if (!consume_command_line_arguments(&disassembler, argc, argv)) return EXIT_FAILURE;

    uint8_t instruction_buffer[4096] = {0};

    FILE *input_file = fopen(disassembler.input_filename, "rb");
    if (!input_file) {
        fprintf(stderr, "Binary file %s is invalid or does not exist\n", disassembler.input_filename);
        return EXIT_FAILURE;
    }

    fseek(input_file, 0, SEEK_END);
    const size_t input_file_size = ftell(input_file);
    rewind(input_file);

    printf("File size: %ld\n", input_file_size);

    if (input_file_size > 4096) {
        fprintf(stderr, "Rom file %s is too big! Rom size: %llu, Max size allowed: %d\n", 
                disassembler.input_filename, (long long unsigned)input_file_size, 4096);
        fclose(input_file);
        return false;
    }
    // Load ROM
    else if (fread(&instruction_buffer, input_file_size, 1, input_file) != 1) {
        fprintf(stderr, "Could not read Rom file %s into buffer\n", 
                disassembler.input_filename);
        fclose(input_file);
        return false;
    }

    bool keep_reading = true;
    long unsigned int I = 0;
    while (keep_reading) {
        const uint16_t current_instruction = (instruction_buffer[I] << 8) | instruction_buffer[I+1];
        if (I >= input_file_size) keep_reading = false;
        else {
            I += 2;
            printf("%lu: %04x: ", I, current_instruction);
            struct DecodedInstruction decoded_intruction = decoded_instruction_from_encoded_instruction(current_instruction);
            instruction_decoded_print(decoded_intruction);
        }
    }

    fclose(input_file);

    return EXIT_SUCCESS;
}