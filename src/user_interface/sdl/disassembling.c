#include "user_interface/sdl/interface.h"
#include "user_interface/instruction_print.h"

static inline void disassembling_user_interface_draw(struct UserInterface *user_interface, struct EmulatedSystem *emulated_system) {
    user_interface->disassembling.message_surface = TTF_RenderText_Blended_Wrapped(
        user_interface->font,
        "Disassembling...", 
        (SDL_Color){255, 255, 255, 255},
        300
    );

    printf("%lu: %04x: ", (long unsigned int)emulated_system->PC, emulated_system->encoded_instruction);
    instruction_decoded_print(emulated_system->decoded_instruction);

    user_interface->disassembling.message = SDL_CreateTextureFromSurface(
        user_interface->renderer,
        user_interface->disassembling.message_surface
    );

    SDL_Rect rectangle = {
        .x = 100,
        .y = 100,
        .w = user_interface->disassembling.message_surface->w,
        .h = user_interface->disassembling.message_surface->h
    };

    SDL_SetRenderDrawColor(user_interface->renderer, 0, 0, 0, 255);
    SDL_RenderCopy(user_interface->renderer, user_interface->disassembling.message, NULL, &rectangle);
}
