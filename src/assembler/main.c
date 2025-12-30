#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "instruction.h"

struct Assembler {
    const char *input_filename;
    const char *output_filename;
};

static bool consume_command_line_arguments(struct Assembler *assembler, int argc, char **argv) {
    static const char *const usage = "Usage: tracuachip8-assembler --input <input_filename> --output <output_filename>\n";

   for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--output", strlen("--output")) == 0) {
            assembler->output_filename = argv[i++];
        }
        else if (strncmp(argv[i], "--input", strlen("--input")) == 0) {
            assembler->input_filename = argv[i++];
        }
    }

    if (assembler->input_filename == NULL) {
        fprintf(stderr, "No input filename.\n");
        fprintf(stderr, usage);
        return false;
    }
    else if (assembler->input_filename == NULL) {
        fprintf(stderr, "No output filename.\n");
        fprintf(stderr, usage);
        return false;
    }

    return true;
}

int main(int argc, char **argv) {
    struct Assembler assembler = {.input_filename = NULL, .output_filename = NULL};

    if (!consume_command_line_arguments(&assembler, argc, argv)) return EXIT_FAILURE;

    return EXIT_SUCCESS;
}