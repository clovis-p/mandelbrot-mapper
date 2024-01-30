//
// Created by clovis on 12/27/23.
//

#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "main.h"
#include "mandelbrot.h"

int quit = 0;

int main()
{
    setbuf(stdout, NULL);

    allocatePixels();

    printf("Waiting for view process to open pipe\n");
    int fd;
    if (mkfifo("/tmp/mandelbrot_pipe", 0666) == -1)
    {
        printf("Error creating pipe: %s\n", strerror(errno));
        perror("mkfifo");
        return 1;
    }
    fd = open("/tmp/mandelbrot_pipe", O_RDWR);
    if (fd == -1)
    {
        printf("Error opening frames pipe: %s\n", strerror(errno));
        return 1;
    }
    int status = 0;

    printf("Initializing OpenCL\n");
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

    printf("\nStarting Mandelbrot set computation\nWaiting for view process to send parameters\n");

    struct timeval start, end;

    //////
    mapMandelbrotSet(cl);
    printf("1: %x\n", pixels[640 * 240 + 320]);
    //////

    uint32_t debugPixels[RESOLUTION_X * RESOLUTION_Y];
    memcpy(debugPixels, pixels, RESOLUTION_X * RESOLUTION_Y * sizeof(uint32_t));
    debugPixels[RESOLUTION_X * RESOLUTION_Y - 1] = 0xFFFFFF;

    while (!quit)
    {
        read(fd, &status, sizeof(int));

        if (status == 1)
        {
            printf("status = %d\n", status);
            gettimeofday(&start, NULL);

            mapMandelbrotSet(cl);
            if (pixels == NULL)
            {
                printf("Error mapping mandelbrot set\n");
                quit = 1;
            }

            printf("2: %x\n", pixels[640 * 240 + 320]);
            write(fd, pixels, RESOLUTION_X * RESOLUTION_Y * sizeof(uint32_t));
            //int pos = 640 * 240 + 320;
            int pos = start.tv_sec * 10000000 % (RESOLUTION_X * RESOLUTION_Y);
            printf("pos = %d\n", pos);
            printf("%x\n", pixels[pos]);

            //memcpy(pixels, pixelsTemp, RESOLUTION_X * RESOLUTION_Y * sizeof(uint32_t));

            gettimeofday(&end, NULL);

            long seconds = (end.tv_sec - start.tv_sec);
            long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);

            printf("Completed frame in %ld.%06lds\n", seconds, micros);
            printf("Waiting for view process to send parameters\n");
        }
    }

    close(fd);
    unlink("/tmp/mandelbrot_pipe_parameters");
    free(pixelsTemp);
    freePixels();
    quitOpenCL(cl);

    return 0;
}