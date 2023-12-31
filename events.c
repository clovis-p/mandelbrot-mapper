//
// Created by clovis on 12/30/23.
//

#include "main.h"

int quit = 0;
SDL_Point mouse;

int handleEvents()
{
    static SDL_Event event;

    while (SDL_WaitEvent(&event) && !quit)
    {
        if (event.type == SDL_QUIT || event.window.event == SDL_WINDOWEVENT_CLOSE)
        {
            quit = 1;
        }

        SDL_GetMouseState(&mouse.x, &mouse.y);
    }

    return 0;
}