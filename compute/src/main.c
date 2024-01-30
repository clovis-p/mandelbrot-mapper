//
// Created by clovis on 12/27/23.
//

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

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

    clock_t start, end;
    double cpu_time_used;

    while (!quit)
    {
        start = clock();

        memcpy(pixels, pixelsTemp, RESOLUTION_X * RESOLUTION_Y * sizeof(uint32_t));

        pixelsTemp = mapMandelbrotSet(cl);
        if (pixelsTemp == NULL)
        {
            printf("Error mapping mandelbrot set\n");
            quit = 1;
        }

        end = clock();
        cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
        printf("Time elapsed: %fms\n", cpu_time_used * 1000);
    }

    freePixels();
    quitOpenCL(cl);

    return 0;
}