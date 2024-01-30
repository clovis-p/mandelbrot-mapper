//
// Created by clovis on 12/27/23.
//

#include <math.h>
#include <time.h>

#include "mandelbrot.h"
#include "main.h"
#include "events.h"

// If a number squared MAX_ITERATIONS times is greater than UNSTABLE_THRESHOLD, it is considered unstable
#define UNSTABLE_THRESHOLD 1000000

// I didn't really think this through, this is a trial and error thing.
// Funny things happen to the colors when this number is changed.
#define FUNNY_NUMBER (-985432)

// 1 = use the number defined above
// 0 = use a random number
#define USE_FUNNY_NUMBER 1

static int isStable(double x);
static DPoint convertScreenPointToMandelbrotPoint(DPoint screenPoint, double viewWidth, double viewHeight, DPoint centerPoint);
static Uint32 isOutsideOfMandelbrotSet(double re, double im);
static SDL_Color assignColorToMandelbrotPoint(DPoint point);
static SDL_Color hexToSDLColor(Uint32 hexValue);

view_s view = {RESOLUTION_X, RESOLUTION_Y,
               ((double)LAST_X_PIXEL / 2), ((double)LAST_Y_PIXEL / 2)};

#define INITIAL_MAX_ITERATIONS 512
int maxIterations = INITIAL_MAX_ITERATIONS;

Uint32* pixels;

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
    cl_uint retNumPlatforms;
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
    cl->colors_buf = clCreateBuffer(cl->context, CL_MEM_WRITE_ONLY, RESOLUTION_X * RESOLUTION_Y * sizeof(Uint32), NULL, &ret);
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

/*
    const int SIZE = 0b1111111111111111111111111111;
    printf("SIZE = %d\n", SIZE);

    int* a = malloc(sizeof(int) * SIZE);
    int* b = malloc(sizeof(int) * SIZE);

    printf("Using OpenCL\n");

    int* c = malloc(sizeof(int) * SIZE);

    FILE *kernelFile = fopen("../addArrays.cl", "r");
    if (!kernelFile)
    {
        printf("Failed to open kernel file\n");
    }

    char* kernelString = malloc(sizeof(char) * MAX_KERNEL_SOURCE_SIZE);
    // Put the contents of the file in kernelString. fread() returns it's size.
    size_t kernelSourceSize = fread(kernelString, 1, MAX_KERNEL_SOURCE_SIZE, kernelFile);
    fclose(kernelFile);

    // Get hardware info
    cl_platform_id platformId = NULL;
    cl_device_id deviceId = NULL;
    cl_uint retNumPlatforms = 0;
    cl_uint retNumDevices = 0;
    cl_int ret = 0;
    printf("Danger début\n");
    ret = clGetPlatformIDs(1, &platformId, &retNumPlatforms);
    printf("Danger fin\n");
    ret = clGetDeviceIDs(platformId, CL_DEVICE_TYPE_DEFAULT, 1, &deviceId, &retNumDevices);

    // Create OpenCL context
    cl_context context = clCreateContext(NULL, 1, &deviceId, NULL, NULL, &ret);

    // Create command queue
    cl_command_queue commandQueue = clCreateCommandQueueWithProperties(context, deviceId, 0, &ret);

    // Create memory buffers
    cl_mem memBuf_A = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(int) * SIZE, NULL, &ret);
    cl_mem memBuf_B = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(int) * SIZE, NULL, &ret);
    cl_mem memBuf_C = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(int) * SIZE, NULL, &ret);

    // Fill the buffers
    ret = clEnqueueWriteBuffer(commandQueue, memBuf_A, CL_TRUE, 0, sizeof(int) * SIZE, a, 0, NULL, NULL);
    ret = clEnqueueWriteBuffer(commandQueue, memBuf_B, CL_TRUE, 0, sizeof(int) * SIZE, b, 0, NULL, NULL);

    // Create and build a program with the kernel source
    cl_program program = clCreateProgramWithSource(context, 1, (const char**)&kernelString, &kernelSourceSize, &ret);
    ret = clBuildProgram(program, 1, &deviceId, NULL, NULL, NULL);

    // Create Kernel and set it's arguments
    cl_kernel kernel = clCreateKernel(program, "addArrays", &ret);
    ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&memBuf_A);
    ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&memBuf_B);
    ret = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)&memBuf_C);

    // Execute kernel
    size_t globalItemSize = SIZE; // Number of items to process
    size_t localItemSize = 64; // Divide the computing into 64 groups
    ret = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, &globalItemSize, &localItemSize, 0, NULL, NULL);

    // Read the "c" memory buffer in the device (GPU) and put it's value in our local "c" variable
    ret = clEnqueueReadBuffer(commandQueue, memBuf_C, CL_TRUE, 0, sizeof(int) * SIZE, c, 0, NULL, NULL);

*/
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
    pixels = (Uint32*)malloc(RESOLUTION_X * RESOLUTION_Y * sizeof(Uint32));
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

Uint32* mapMandelbrotSet(cl_s* cl)
{
    //SDL_Texture* mandelbrotTexture = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888,
    //                                                   SDL_TEXTUREACCESS_TARGET, RESOLUTION_X, RESOLUTION_Y);
/*
    if (mandelbrotTexture == NULL)
    {
        printf("Error creating texture: %s\n", SDL_GetError());
    }
*/
    SDL_Point currentPixel = {0, 0};
    DPoint currentPixelD = {0, 0,};
    DPoint mandelbrotCoords = {0, 0};

    SDL_Color color = {0, 0, 0, 255};

    int funnyNumber;

    if (!USE_FUNNY_NUMBER)
    {
        srand(time(NULL));
        funnyNumber = rand() % 1000000000 - 500000000;
        //printf("Funny number = %d\n", funnyNumber);
    }
    else
    {
        funnyNumber = FUNNY_NUMBER;
    }

    cl_int ret;

    // on essaye tu de tasser ca dans l'init?
    ret = clEnqueueWriteBuffer(cl->commandQueue, cl->viewParams_buf, CL_TRUE, 0,
                         4 * sizeof(double), &view, 0, NULL, NULL);
    if (ret != CL_SUCCESS)
    {
        printf("Error writing to buffers\n");
        quit = 1;
        return NULL;
    }

    // Execute kernel
    size_t globalWorkSize[1] = {cl->globalPixelCount};
    size_t localWorkSize[1] = {cl->localPixelCount};
    ret = clEnqueueNDRangeKernel(cl->commandQueue, cl->kernel, 1, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
    if (ret != CL_SUCCESS)
    {
        printf("Error executing kernel\n");
        quit = 1;
        return NULL;
    }

    // Read the results
    ret = clEnqueueReadBuffer(cl->commandQueue, cl->colors_buf, CL_TRUE, 0,
                        RESOLUTION_X * RESOLUTION_Y * sizeof(Uint32), pixels, 0,
                        NULL, NULL);
    if (ret != CL_SUCCESS)
    {
        printf("Error reading from colors_buf\n");
        quit = 1;
        return NULL;
    }

    //SDL_UpdateTexture(mandelbrotTexture, NULL, pixels, RESOLUTION_X * sizeof(Uint32));

    return pixels;
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

static DPoint convertScreenPointToMandelbrotPoint(DPoint screenPoint, double viewWidth, double viewHeight, DPoint centerPoint)
{
    DPoint mandelbrotPoint;

    mandelbrotPoint.x = (double)screenPoint.x / LAST_X_PIXEL * (4 * (viewWidth / RESOLUTION_X)) - (2 * (viewWidth / RESOLUTION_X)) + (centerPoint.x / LAST_X_PIXEL * 4 - 2);

    double yRange = (double)RESOLUTION_Y / RESOLUTION_X;

    mandelbrotPoint.y = (double)screenPoint.y / LAST_Y_PIXEL * (4 * (viewHeight / RESOLUTION_Y)) * yRange - (2 * (viewHeight / RESOLUTION_Y)) * yRange + (centerPoint.y / LAST_Y_PIXEL * 4 - 2);

    return mandelbrotPoint;
}

static Uint32 isOutsideOfMandelbrotSet(double re, double im)
{
    double zReal = 0.0;
    double zImag = 0.0;

    for (int i = 0; i < maxIterations && !quit; ++i)
    {
        double zRealTemp = zReal * zReal - zImag * zImag + re;
        double zImagTemp = 2 * zReal * zImag + im;

        zReal = zRealTemp;
        zImag = zImagTemp;

        // If the magnitude of z becomes too large, consider it unstable
        if (zReal * zReal + zImag * zImag > UNSTABLE_THRESHOLD)
        {
            // If the point is not in the Mandelbrot set, we return the number of iterations it took before the
            // unstable threshold was passed. This will be used to assign a color to the current pixel.
            return i;
        }
    }

    return 0; // Point in Mandelbrot set
}

// Takes the number of iterations it took for a point to become unstable and returns a color
static SDL_Color assignColorToMandelbrotPoint(DPoint point)
{
    Uint32 hexValue = (FUNNY_NUMBER * isOutsideOfMandelbrotSet(point.x, point.y) * 256 / INITIAL_MAX_ITERATIONS) % 0xFFFFFF;

    return hexToSDLColor(hexValue);
}

static SDL_Color hexToSDLColor(Uint32 hexValue)
{
    SDL_Color color;
    color.r = (hexValue >> 16) & 0xFF; // Extracting red component
    color.g = (hexValue >> 8) & 0xFF;  // Extracting green component
    color.b = hexValue & 0xFF;         // Extracting blue component
    color.a = 255;                     // Setting alpha channel to opaque (255)

    return color;
}