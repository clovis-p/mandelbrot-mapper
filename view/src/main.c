//
// Created by clovis on 12/27/23.
//

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "main.h"
#include "events.h"
#include "mandelbrot.h"

//uint32_t* pixels;

int sendViewParams();

int viewParamsChanged = 0;

int main()
{
    setbuf(stdout, NULL);

    printf("Starting view process\n");

    printf("Waiting for compute process to open pipe\n");
    int fd;
    /*
    if (mkfifo("/tmp/mandelbrot_pipe", 0666) == -1)
    {
        printf("Error creating pipe: %s\n", strerror(errno));
        perror("mkfifo");
        return 1;
    }*/
    fd = open("/tmp/mandelbrot_pipe", O_RDWR);
    if (fd == -1)
    {
        printf("Error opening frames pipe: %s\n", strerror(errno));
        perror("mkfifo");
        return 1;
    }
    int status = 0;

    printf("Initializing SDL\n");

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
    SDL_Thread* viewParamsThread = SDL_CreateThread(sendViewParams, "View Params Thread", NULL);

    //allocatePixels();

    mandelbrotTexture = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
                                          RESOLUTION_X, RESOLUTION_Y);

    while (!quit)
    {
        //ticksBefore = SDL_GetTicks();

        uint32_t pixels[RESOLUTION_X * RESOLUTION_Y];

        ssize_t totalBytesRead = 0;
        ssize_t bytesRead = 0;
        while (totalBytesRead < RESOLUTION_X * RESOLUTION_Y * sizeof(uint32_t))
        {
            bytesRead = read(fd, pixels + totalBytesRead / sizeof(uint32_t), RESOLUTION_X * RESOLUTION_Y * sizeof(uint32_t) - totalBytesRead);
            if (bytesRead == -1)
            {
                printf("Error reading from pipe: %s\n", strerror(errno));
            }

            totalBytesRead += bytesRead;
        }

        //uint32_t debugPixels[RESOLUTION_X * RESOLUTION_Y];
        //memcpy(debugPixels, pixels, RESOLUTION_X * RESOLUTION_Y * sizeof(uint32_t));

        SDL_UpdateTexture(mandelbrotTexture, NULL, pixels, RESOLUTION_X * sizeof(uint32_t));
        SDL_RenderCopy(ren, mandelbrotTexture, NULL, NULL);
        SDL_RenderPresent(ren);

        //elapsedTime = SDL_GetTicks() - ticksBefore;
        //printf("Frame time: %d ms\n", elapsedTime);
    }

    close(fd);
    unlink("/tmp/mandelbrot_pipe");

    SDL_WaitThread(eventThread, NULL);
    SDL_WaitThread(viewParamsThread, NULL);

    SDL_DestroyTexture(mandelbrotTexture);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}

int sendViewParams()
{
    printf("Starting view params thread\n");

    int fd = open("/tmp/mandelbrot_pipe_parameters", O_WRONLY);
    if (fd == -1)
    {
        printf("Error opening parameters pipe: %s\n", strerror(errno));
        perror("mkfifo");
        return -1;
    }

    while (!quit)
    {
        if (viewParamsChanged)
        {
            ssize_t totalBytesSent = 0;
            ssize_t bytesSent = 0;
            while (totalBytesSent < 4 * sizeof(double))
            {
                bytesSent = write(fd, &view, 4 * sizeof(double) - totalBytesSent);
                if (bytesSent == -1)
                {
                    printf("Error writing to pipe: %s\n", strerror(errno));
                }

                totalBytesSent += bytesSent;
            }

            viewParamsChanged = 0;
        }
    }

    close(fd);
    unlink("/tmp/mandelbrot_pipe_parameters");

    return 0;
}