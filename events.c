//
// Created by clovis on 12/30/23.
//

#include "main.h"

int quit = 0;

void handleEvents()
{
    static SDL_Event event;

    if (SDL_WaitEvent(&event))
    {
        if (event.type == SDL_QUIT || event.window.event == SDL_WINDOWEVENT_CLOSE)
        {
            quit = 1;
        }
    }
}