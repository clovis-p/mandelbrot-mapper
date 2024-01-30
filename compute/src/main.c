//
// Created by clovis on 12/27/23.
//

#include <stdio.h>

#include "main.h"
#include "events.h"
#include "mandelbrot.h"

int mandelbrotCompute(void* data);
//void* mandelbrotCompute(void* data);
SDL_mutex* openCLMutex;

int lockRendering = 0;
int openCLInitialized = 0;
int sdlInitialized = 0;

int main()
{
    setbuf(stdout, NULL);

    printf("compute\n");

    openCLMutex = SDL_CreateMutex();
    if (openCLMutex == NULL)
    {
        printf("Error creating mutex\n");
        return 1;
    }
    SDL_LockMutex(openCLMutex);

    SDL_Thread* mandelbrotComputeThread = SDL_CreateThread(mandelbrotCompute, "Mandelbrot Compute Thread", NULL);

    while (!openCLInitialized)
    {
        SDL_Delay(50);
        printf(".");
    }
    printf("\n");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0)
    {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_SetHint("SDL_HINT_RENDERER_SOFTWARE", "1");
    SDL_SetHint("SDL_HINT_RENDERER_DRIVER", "software");

    SDL_Window* win = SDL_CreateWindow("Mandelbrot Mapper", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                       RESOLUTION_X, RESOLUTION_Y, SDL_WINDOW_SHOWN);

    if (win == NULL)
    {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    int numRenderDrivers = SDL_GetNumRenderDrivers();
    if (numRenderDrivers == 0)
    {
        printf("SDL_GetNumRenderDrivers Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    for (int i = 0; i < numRenderDrivers; ++i)
    {
        SDL_RendererInfo info;
        SDL_GetRenderDriverInfo(i, &info);
        printf("Driver %d: %s\n", i, info.name);
    }

    SDL_Renderer* ren;
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);

    if (ren == NULL)
    {
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    printf("SDL initialized\n");
    SDL_UnlockMutex(openCLMutex);
    sdlInitialized = 1;

    Uint32 ticksBefore;
    Uint32 elapsedTime;
    SDL_Texture* mandelbrotTexture;

    //SDL_Thread* eventThread = SDL_CreateThread(handleEvents, "Event Thread", NULL);

    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderClear(ren);
    SDL_RenderPresent(ren);

    while (!quit)
    {
        SDL_Delay(16);
    }

    //SDL_WaitThread(eventThread, NULL);
    //pthread_join(mandelbrotComputeID, NULL);
    SDL_WaitThread(mandelbrotComputeThread, NULL);

    SDL_DestroyMutex(openCLMutex);

    return 0;
}

int mandelbrotCompute(void* data)
{
    allocatePixels();

    cl_s* cl = initOpenCL();

    if (cl == NULL)
    {
        printf("Error initializing OpenCL\n");
        quit = 1;
    }

    Uint32* pixelsTemp = (Uint32*)malloc(RESOLUTION_X * RESOLUTION_Y * sizeof(Uint32));
    if (pixelsTemp == NULL)
    {
        printf("Error allocating memory for pixelsTemp\n");
        quit = 1;
    }

    printf("OpenCL initialized\n");
    openCLInitialized = 1;
    SDL_UnlockMutex(openCLMutex);

    while (!sdlInitialized)
    {
        SDL_Delay(50);
        printf("-");
    }
    printf("\nStarting Mandelbrot set computation\n");

    Uint32 ticks1, ticks2;

    while (!quit)
    {
        ticks1 = SDL_GetTicks();

        lockRendering = 1;
        memcpy(pixels, pixelsTemp, RESOLUTION_X * RESOLUTION_Y * sizeof(Uint32));
        lockRendering = 0;

        pixelsTemp = mapMandelbrotSet(cl);
        if (pixelsTemp == NULL)
        {
            printf("Error mapping mandelbrot set\n");
            quit = 1;
        }

        ticks2 = SDL_GetTicks();
        printf("Elapsed time: %dms\n", ticks2 - ticks1);
    }

    freePixels();
    quitOpenCL(cl);

    return 0;
}