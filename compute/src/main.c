//
// Created by clovis on 12/27/23.
//

#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>

#include "main.h"
#include "mandelbrot.h"

int quit = 0;

void* getViewParams();

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

    pthread_t viewParamsThread;
    int ret = pthread_create(&viewParamsThread, NULL, getViewParams, NULL);

    printf("\nStarting Mandelbrot set computation\nWaiting for view process to send parameters\n");

    struct timeval start, end;

    //////
    mapMandelbrotSet(cl);
    printf("1: %x\n", pixels[640 * 240 + 320]);
    //////

    uint32_t debugPixels[RESOLUTION_X * RESOLUTION_Y];
    memcpy(debugPixels, pixels, RESOLUTION_X * RESOLUTION_Y * sizeof(uint32_t));
    debugPixels[RESOLUTION_X * RESOLUTION_Y - 1] = 0xFFFFFF;

    status = 1;

    while (!quit)
    {
        //read(fd, &status, sizeof(int));

        printf("Let's go!\n");

        if (status == 1) {
            //printf("status = %d\n", status);
            gettimeofday(&start, NULL);

            mapMandelbrotSet(cl);
            if (pixels == NULL) {
                printf("Error mapping mandelbrot set\n");
                quit = 1;
            }

            //printf("2: %x\n", pixels[640 * 240 + 320]);
            memcpy(debugPixels, pixels, RESOLUTION_X * RESOLUTION_Y * sizeof(uint32_t));

            ssize_t totalBytesSent = 0;
            ssize_t bytesSent = 0;
            while (totalBytesSent < RESOLUTION_X * RESOLUTION_Y * sizeof(uint32_t))
            {
                bytesSent = write(fd, pixels + totalBytesSent / sizeof(uint32_t), RESOLUTION_X * RESOLUTION_Y * sizeof(uint32_t) - totalBytesSent);
                if (bytesSent == -1)
                {
                    printf("Error writing to pipe: %s\n", strerror(errno));
                }
                //else
                //{
                //    printf("Sent %ld (+%ld) bytes for a total of %ld bytes\n", totalBytesSent, bytesSent, (RESOLUTION_X * RESOLUTION_Y * sizeof(uint32_t)));
                //}

                totalBytesSent += bytesSent;
            }

            gettimeofday(&end, NULL);

            long seconds = (end.tv_sec - start.tv_sec);
            long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);

            printf("Completed frame in %ld.%06lds\n", seconds, micros);
            //printf("Waiting for view process to send parameters\n");
            //status = 0;
        }

        //read(fd, &status, sizeof(int));
    }

    close(fd);
    unlink("/tmp/mandelbrot_pipe_parameters");
    free(pixelsTemp);
    freePixels();
    quitOpenCL(cl);

    return 0;
}

void* getViewParams()
{
    printf("Starting view params thread\n");

    int fd;
    if (mkfifo("/tmp/mandelbrot_pipe_parameters", 0666) == -1)
    {
        printf("Error creating parameters pipe: %s\n", strerror(errno));
        perror("mkfifo");
        return NULL;
    }
    fd = open("/tmp/mandelbrot_pipe_parameters", O_RDONLY);
    if (fd == -1)
    {
        printf("Error opening parameters pipe: %s\n", strerror(errno));
        perror("mkfifo");
        return NULL;
    }

    while (!quit)
    {
        ssize_t totalBytesRead = 0;
        ssize_t bytesRead = 0;
        while (totalBytesRead < 4 * sizeof(double))
        {
            bytesRead = read(fd, &view, 4 * sizeof(double) - totalBytesRead);
            if (bytesRead == -1)
            {
                printf("Error writing to pipe: %s\n", strerror(errno));
            }
            totalBytesRead += bytesRead;
        }
    }

    close(fd);
    return NULL;
}