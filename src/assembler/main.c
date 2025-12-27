#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

struct Assembler {
    const char *input_filename;
    const char *output_filename;
};

bool consume_command_line_arguments(struct Assembler *assembler, int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_filename>\n", argv[0]);
        return false;
    }

   for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--output", strlen("--output")) == 0) {
            assembler->input_filename = argv[i++];
        }
    }

    return true;
}

int main(int argc, char **argv) {
    struct Assembler assembler;

    if (!consume_command_line_arguments(&assembler, argc, argv)) return EXIT_FAILURE;

    return EXIT_SUCCESS;
}