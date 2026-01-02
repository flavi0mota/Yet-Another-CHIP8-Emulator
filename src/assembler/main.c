#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "instruction.h"

struct Assembler {
    const char *input_filename;
    const char *output_filename;
};

static const char *const usage = "Usage: tracuachip8-assembler --input <input_filename> --output <output_filename>\n";

static bool consume_command_line_arguments(struct Assembler *assembler, int argc, char **argv) {
   for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--output", strlen("--output")) == 0) {
            assembler->output_filename = argv[++i];
        }
        else if (strncmp(argv[i], "--input", strlen("--input")) == 0) {
            assembler->input_filename = argv[++i];
        }
    }

    if (assembler->input_filename == NULL) {
        fprintf(stderr, "No input filename.\n");
        return false;
    }
    else if (assembler->input_filename == NULL) {
        fprintf(stderr, "No output filename.\n");
        return false;
    }

    return true;
}

int main(int argc, char **argv) {
    struct Assembler assembler = {0};

    if (!consume_command_line_arguments(&assembler, argc, argv)) {
        fprintf(stderr, usage);
        return EXIT_FAILURE;
    }

    char source_line[100] = {0};

    FILE *input_file = fopen(assembler.input_filename, "rb");
    if (!input_file) {
        fprintf(stderr, "Source file %s is invalid or does not exist\n", assembler.input_filename);
        return EXIT_FAILURE;
    }

    while (fgets(source_line, sizeof(source_line), input_file) != NULL) {
        struct DecodedInstruction decoded_instruction = decoded_instruction_from_string(source_line);

        // TODO: translate labels as operands to memory adresses, constants, variables, etc.

        printf("%s", source_line);

        uint16_t encoded_instruction = encoded_instruction_from_decoded_instruction(decoded_instruction);
        printf("%04x\n", encoded_instruction);
    }

    return EXIT_SUCCESS;
}