#include <stdbool.h>
#include <time.h>

#include <SDL2/SDL.h>

#include "chip8.h"
#include "input.h"

int main(int argc, char **argv) {
    if (argc < 2) {
       fprintf(stderr, "Usage: %s <rom_name>\n", argv[0]);
       exit(EXIT_FAILURE);
    }

    // Configuração inicial
    config_t config = {0};
    if (!set_config_from_args(&config, argc, argv)) exit(EXIT_FAILURE);

    sdl_t sdl = {0};
    if (!init_sdl(&sdl, &config)) exit(EXIT_FAILURE);

    // Setar a máquina
    chip8_t chip8 = {0};
    const char *rom_name = argv[1];
    if (!init_chip8(&chip8, config, rom_name)) exit(EXIT_FAILURE);

    clear_screen(sdl, config);

    srand(time(NULL));

    // Loop do emulador
    while (chip8.state != QUIT) {
        handle_input(&chip8, &config);
        if (chip8.state != PAUSE) process_frame(&chip8, &config, &sdl);
    }
    final_cleanup(sdl); 

    exit(EXIT_SUCCESS);
}