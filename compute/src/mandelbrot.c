//
// Created by clovis on 12/27/23.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mandelbrot.h"
#include "main.h"

view_s view = {RESOLUTION_X, RESOLUTION_Y,
               ((double)LAST_X_PIXEL / 2), ((double)LAST_Y_PIXEL / 2)};

#define INITIAL_MAX_ITERATIONS 512
int maxIterations = INITIAL_MAX_ITERATIONS;

uint32_t* pixels;

#define MAX_KERNEL_SOURCE_SIZE (10000)

cl_s* initOpenCL()
{
    cl_s* cl = (cl_s*)malloc(sizeof(cl_s));
    if (cl == NULL)
    {
        printf("Error allocating memory for cl_s\n");
        quit = 1;
        return NULL;
    }

    // Load kernel source file and put its contents in the "kernelString" string
    FILE *kernelFile = fopen("../src/mandelbrot-kernel.cl", "r");
    if (kernelFile == NULL)
    {
        printf("Error opening kernel file\n");
        quit = 1;
        return NULL;
    }

    cl->kernelString = (char*)malloc(MAX_KERNEL_SOURCE_SIZE * sizeof(char));
    if (cl->kernelString == NULL)
    {
        printf("Error allocating memory for kernelString\n");
        quit = 1;
        return NULL;
    }
    size_t kernelSourceSize = fread(cl->kernelString, 1, MAX_KERNEL_SOURCE_SIZE, kernelFile);
    if (kernelSourceSize == 0)
    {
        printf("Error reading kernel file\n");
        quit = 1;
        return NULL;
    }
    fclose(kernelFile);

    // Get hardware info
    cl->platformId = NULL;
    cl_platform_id platformId = NULL;
    cl->deviceId = NULL;
    cl_uint retNumDevices;
    cl_int ret;
    printf("Danger zone!!\n");

    //ret = clGetPlatformIDs(1, &platformId, &retNumPlatforms);

    cl_uint numPlatforms;
    ret = clGetPlatformIDs(0, NULL, &numPlatforms);
    if (ret != CL_SUCCESS || numPlatforms <= 0)
    {
        printf("Error: No platforms found. Check OpenCL installation.\n");
        quit = 1;
        return NULL;
    }

    cl_platform_id* platformIds = (cl_platform_id*)malloc(sizeof(cl_platform_id) * numPlatforms);
    ret = clGetPlatformIDs(numPlatforms, platformIds, NULL);
    if (ret != CL_SUCCESS)
    {
        printf("Error: Failed to get platformIDs.\n");
        quit = 1;
        return NULL;
    }

    // Use the first platform
    platformId = platformIds[0];
    free(platformIds);

    printf("We good!\n");
    cl->platformId = platformId;
    printf("Moved platform ID\n");
    ret = clGetDeviceIDs(cl->platformId, CL_DEVICE_TYPE_DEFAULT, 1, &cl->deviceId, &retNumDevices);
    if (ret != CL_SUCCESS)
    {
        printf("Error getting device ID\n");
        quit = 1;
        return NULL;
    }

    // Create an OpenCL context and command queue
    cl->context = clCreateContext(NULL, 1, &cl->deviceId, NULL, NULL, &ret);
    if (ret != CL_SUCCESS)
    {
        printf("Error creating context\n");
        quit = 1;
        return NULL;
    }
    cl->commandQueue = clCreateCommandQueueWithProperties(cl->context, cl->deviceId, 0, &ret);
    if (ret != CL_SUCCESS)
    {
        printf("Error creating command queue\n");
        quit = 1;
        return NULL;
    }

    // Create buffers and fill those that only need to be filled once
    cl->screenPoints_buf = clCreateBuffer(cl->context, CL_MEM_READ_ONLY, RESOLUTION_X * RESOLUTION_Y * 2 * sizeof(double), NULL, &ret);
    cl->viewParams_buf = clCreateBuffer(cl->context, CL_MEM_READ_ONLY, 4 * sizeof(double), NULL, &ret);
    //cl->funnyNumber_buf = clCreateBuffer(cl->context, CL_MEM_READ_ONLY, sizeof(int), NULL, &ret);
    cl->colors_buf = clCreateBuffer(cl->context, CL_MEM_WRITE_ONLY, RESOLUTION_X * RESOLUTION_Y * sizeof(uint32_t), NULL, &ret);
    if (ret != CL_SUCCESS)
    {
        printf("Error creating buffers\n");
        quit = 1;
        return NULL;
    }

    double screenPoints[RESOLUTION_X * RESOLUTION_Y * 2];
    for (int y = 0; y < RESOLUTION_Y; y++)
    {
        for (int x = 0; x < RESOLUTION_X; x++)
        {
            int pixelNb = x + y * RESOLUTION_X;
            screenPoints[pixelNb * 2] = (double)x;
            screenPoints[pixelNb * 2 + 1] = (double)y;
        }
    }
    ret = clEnqueueWriteBuffer(cl->commandQueue, cl->screenPoints_buf, CL_TRUE, 0,
                         RESOLUTION_X * RESOLUTION_Y * 2 * sizeof(double), &screenPoints, 0,
                         NULL, NULL);
    if (ret != CL_SUCCESS)
    {
        printf("Error writing to screenPoints_buf\n");
        quit = 1;
        return NULL;
    }

    // Create and build a program with the kernel source, then create a kernel from it and set its arguments
    cl->program = clCreateProgramWithSource(cl->context, 1, (const char**)&cl->kernelString, (const size_t*)&kernelSourceSize, &ret);
    if (ret != CL_SUCCESS)
    {
        printf("Error creating program\n");
        quit = 1;
        return NULL;
    }
    ret = clBuildProgram(cl->program, 1, &cl->deviceId, NULL, NULL, NULL);
    if (ret != CL_SUCCESS)
    {
        printf("Error building program\n");
        printf("Getting build log...\n");

        size_t len;
        char buffer[2048];
        clGetProgramBuildInfo(cl->program, cl->deviceId, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        printf("%s\n", buffer);

        quit = 1;
        return NULL;
    }

    cl->kernel = clCreateKernel(cl->program, "computeMandelbrotPixel", &ret);
    if (ret != CL_SUCCESS)
    {
        printf("Error creating kernel\n");
        quit = 1;
        return NULL;
    }

    ret = clSetKernelArg(cl->kernel, 0, sizeof(cl_mem), (void*)&cl->screenPoints_buf);
    if (ret != CL_SUCCESS)
    {
        printf("Error setting kernel argument 0\n");
        quit = 1;
        return NULL;
    }

    ret = clSetKernelArg(cl->kernel, 1, sizeof(cl_mem), (void*)&cl->viewParams_buf);
    if (ret != CL_SUCCESS)
    {
        printf("Error setting kernel argument 1\n");
        quit = 1;
        return NULL;
    }

    ret = clSetKernelArg(cl->kernel, 2, sizeof(cl_mem), (void*)&cl->colors_buf);
    if (ret != CL_SUCCESS)
    {
        printf("Error setting kernel argument 2\n");
        quit = 1;
        return NULL;
    }

    cl->globalPixelCount = RESOLUTION_X * RESOLUTION_Y;
    cl->localPixelCount = 64;

    return cl;
}

void quitOpenCL(cl_s* cl)
{
    clReleaseMemObject(cl->screenPoints_buf);
    clReleaseMemObject(cl->viewParams_buf);
    clReleaseMemObject(cl->colors_buf);

    clReleaseKernel(cl->kernel);
    clReleaseContext(cl->context);
    clReleaseProgram(cl->program);
    clReleaseCommandQueue(cl->commandQueue);
    clReleaseContext(cl->context);

    free(cl->kernelString);
    free(cl);
}

void allocatePixels()
{
    pixels = (uint32_t*)malloc(RESOLUTION_X * RESOLUTION_Y * sizeof(uint32_t));
    if (pixels == NULL)
    {
        printf("Error allocating memory for pixels\n");
        quit = 1;
    }
}

void freePixels()
{
    free(pixels);
}

void mapMandelbrotSet(cl_s* cl)
{
    uint32_t debugPixels[RESOLUTION_X * RESOLUTION_Y];

    cl_int ret;

    ret = clEnqueueWriteBuffer(cl->commandQueue, cl->viewParams_buf, CL_TRUE, 0,
                         4 * sizeof(double), &view, 0, NULL, NULL);
    if (ret != CL_SUCCESS)
    {
        printf("Error writing to buffers\n");
        quit = 1;
    }

    // Execute kernel
    size_t globalWorkSize[1] = {cl->globalPixelCount};
    size_t localWorkSize[1] = {cl->localPixelCount};
    ret = clEnqueueNDRangeKernel(cl->commandQueue, cl->kernel, 1, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
    if (ret != CL_SUCCESS)
    {
        printf("Error executing kernel\n");
        quit = 1;
    }

    // Read the results
    ret = clEnqueueReadBuffer(cl->commandQueue, cl->colors_buf, CL_TRUE, 0,
                        RESOLUTION_X * RESOLUTION_Y * sizeof(uint32_t), debugPixels, 0,
                        NULL, NULL);
    if (ret != CL_SUCCESS)
    {
        printf("Error reading from colors_buf\n");
        quit = 1;
    }
/*
    for (int i = 0; i < 100; i++)
    {
        printf("%x - ", debugPixels[i]);
    }
*/
    memcpy(pixels, debugPixels, RESOLUTION_X * RESOLUTION_Y * sizeof(uint32_t));

/*
    for (int i = 0; i < 15; i++)
    {
        //int j = i * 128;
        printf("Pixel %d: %x\n", i, pixels[i]);
    }
*/
    //printf("0x%x\n", pixels[240 * 640 + 320]);
}

void zoomInViewSize()
{
    view.viewWidth /= 1.25;
    view.viewHeight /= 1.25;

    maxIterations *= 1.05;
    printf("maxIterations = %d\n", maxIterations);
    printf("View: w = %lf, h = %lf\n", view.viewWidth, view.viewHeight);
}

void zoomOutViewSize()
{
    view.viewWidth *= 1.25;
    view.viewHeight *= 1.25;

    maxIterations /= 1.05;
    printf("maxIterations = %d\n", maxIterations);
    printf("View: w = %lf, h = %lf\n", view.viewWidth, view.viewHeight);
}