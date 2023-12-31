//
// Created by clovis on 12/27/23.
//

#include <stdio.h>

#include "main.h"
#include "events.h"
#include "mandelbrot.h"

int main()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* win = SDL_CreateWindow("Mandelbrot Mapper", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                       RESOLUTION_X, RESOLUTION_Y, SDL_WINDOW_SHOWN);

    if (win == NULL)
    {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (ren == NULL)
    {
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    Uint32 ticksBefore;
    Uint32 elapsedTime;
    SDL_Texture* mandelbrotTexture;

    SDL_Thread* eventThread = SDL_CreateThread(handleEvents, "Event Thread", NULL);

    while (!quit)
    {
        ticksBefore = SDL_GetTicks();
        mandelbrotTexture = mapMandelbrotSet(ren);
        elapsedTime = SDL_GetTicks() - ticksBefore;
        printf("%dms\n", elapsedTime);

        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, mandelbrotTexture, NULL, NULL);
        SDL_RenderPresent(ren);
    }

    SDL_WaitThread(eventThread, NULL);

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}