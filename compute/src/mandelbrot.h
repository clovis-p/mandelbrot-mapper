//
// Created by clovis on 12/27/23.
//

#ifndef MANDELBROT_MAPPER_MANDELBROT_H
#define MANDELBROT_MAPPER_MANDELBROT_H

#include <CL/cl.h>

#include "main.h"

void allocatePixels();
void freePixels();

void zoomInViewSize();
void zoomOutViewSize();

typedef struct
{
    double real;
    double imaginary;
} complex_s;

typedef struct
{
    double viewWidth;
    double viewHeight;
    DPoint centerPoint;
} view_s;

typedef struct
{
    cl_platform_id platformId;
    cl_device_id deviceId;
    cl_uint retNumPlatforms;
    cl_uint retNumDevices;

    cl_context context;
    cl_command_queue commandQueue;
    cl_kernel kernel;
    cl_program program;

    char* kernelString;

    cl_mem screenPoints_buf;
    cl_mem viewParams_buf;
    cl_mem colors_buf;

    size_t globalPixelCount;
    size_t localPixelCount;
} cl_s;

cl_s* initOpenCL();
void quitOpenCL(cl_s* cl);
void mapMandelbrotSet(cl_s* cl);

extern view_s view;
extern uint32_t* pixels;

#endif //MANDELBROT_MAPPER_MANDELBROT_H