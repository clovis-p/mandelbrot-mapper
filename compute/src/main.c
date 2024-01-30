//
// Created by clovis on 12/27/23.
//

#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "main.h"
#include "mandelbrot.h"

int quit = 0;

int main()
{
    setbuf(stdout, NULL);

    printf("Starting compute process\n");

    allocatePixels();

    cl_s* cl = initOpenCL();

    if (cl == NULL)
    {
        printf("Error initializing OpenCL\n");
        quit = 1;
    }

    uint32_t* pixelsTemp = (uint32_t*)malloc(RESOLUTION_X * RESOLUTION_Y * sizeof(uint32_t));
    if (pixelsTemp == NULL)
    {
        printf("Error allocating memory for pixelsTemp\n");
        quit = 1;
    }

    printf("\nStarting Mandelbrot set computation\n");

    struct timeval start, end;

    while (!quit)
    {
        gettimeofday(&start, NULL);

        memcpy(pixels, pixelsTemp, RESOLUTION_X * RESOLUTION_Y * sizeof(uint32_t));

        pixelsTemp = mapMandelbrotSet(cl);
        if (pixelsTemp == NULL)
        {
            printf("Error mapping mandelbrot set\n");
            quit = 1;
        }

        gettimeofday(&end, NULL);

        long seconds = (end.tv_sec - start.tv_sec);
        long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);

        printf("Completed loop in %ld.%06lds\n", seconds, micros);
    }

    free(pixelsTemp);
    freePixels();
    quitOpenCL(cl);

    return 0;
}