#include "user_interface/sdl/interface.h"

static void pause_menu_user_interface_draw(struct UserInterface *user_interface) {
    SDL_Rect rectangle = {
        .x = 100,
        .y = 100,
        .w = user_interface->pause_menu.message_surface->w,
        .h = user_interface->pause_menu.message_surface->h
    };
    SDL_SetRenderDrawColor(user_interface->renderer, 0, 0, 0, 255);
    SDL_RenderClear(user_interface->renderer);
    SDL_RenderCopy(user_interface->renderer, user_interface->pause_menu.message, NULL, &rectangle);
    SDL_RenderPresent(user_interface->renderer);
}