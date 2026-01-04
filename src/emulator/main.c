#include <stdbool.h>
#include <time.h> // time()

#include "emulator.h"

bool consume_command_line_arguments(struct Emulator *emulator, int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <rom_name>\n", argv[0]);
        return false;
    }

   for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--scale-factor", strlen("--scale-factor")) == 0) {
            i++;
            emulator->user_interface.scale_factor = (uint32_t)strtol(argv[i], NULL, 10);
        }
    }
    emulator_load_rom(emulator, argv[1]);
    return true;
}

int main(int argc, char **argv) {
    struct Emulator emulator;

    emulator_initialize(&emulator);

    if (!consume_command_line_arguments(&emulator, argc, argv)) return EXIT_FAILURE;
    else {
        srand(time(NULL));

        while (emulator.emulated_system.state != QUIT) {
            emulator_update(&emulator);
        }
        emulator_destroy(&emulator); 
        return EXIT_SUCCESS;
    }
}