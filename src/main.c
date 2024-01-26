//
// Created by clovis on 12/27/23.
//

#include <stdio.h>
#include <pthread.h>

#include "main.h"
#include "events.h"
#include "mandelbrot.h"

void* mandelbrotCompute(void* data);

int lockRendering = 0;

int main()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* win = SDL_CreateWindow("Mandelbrot Mapper", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                       RESOLUTION_X, RESOLUTION_Y, WINDOW_FLAGS);

    if (win == NULL)
    {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);

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

    pthread_t mandelbrotComputeID;
    pthread_create(&mandelbrotComputeID, NULL, mandelbrotCompute, NULL);

    //SDL_Thread* eventThread = SDL_CreateThread(handleEvents, "Event Thread", NULL);

    while (!quit)
    {
        if (!lockRendering)
        {
            SDL_UpdateTexture(mandelbrotTexture, NULL, pixels, RESOLUTION_X * sizeof(Uint32));
        }

        SDL_RenderCopy(ren, mandelbrotTexture, NULL, NULL);
        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }


    //SDL_WaitThread(eventThread, NULL);
    pthread_join(mandelbrotComputeID, NULL);

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}

void* mandelbrotCompute(void* data)
{
    allocatePixels();

    cl_s* cl = initOpenCL();

    Uint32* pixelsTemp = (Uint32*)malloc(RESOLUTION_X * RESOLUTION_Y * sizeof(Uint32));
    if (pixelsTemp == NULL)
    {
        printf("Error allocating memory for pixelsTemp\n");
        quit = 1;
    }

    while (!quit)
    {
        pixelsTemp = mapMandelbrotSet(cl);
        lockRendering = 1;
        memcpy(pixels, pixelsTemp, RESOLUTION_X * RESOLUTION_Y * sizeof(Uint32));
        lockRendering = 0;
    }

    freePixels();
    quitOpenCL(cl);

    pthread_exit(NULL);
}