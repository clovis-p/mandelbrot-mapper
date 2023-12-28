//
// Created by clovis on 12/27/23.
//

#include <stdio.h>

#include <SDL2/SDL.h>

#include "main.h"

int main()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* win = SDL_CreateWindow("Mandelbrot Mapper", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                       640,480,SDL_WINDOW_SHOWN);

    if (win == NULL)
    {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Event event;

    while (1)
    {
        if (SDL_WaitEvent(&event))
        {
            if (event.type == SDL_QUIT || event.window.event == SDL_WINDOWEVENT_CLOSE)
            {
                break;
            }
        }
    }

    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}