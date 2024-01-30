//
// Created by clovis on 12/27/23.
//

#ifndef MANDELBROT_MAPPER_MANDELBROT_H
#define MANDELBROT_MAPPER_MANDELBROT_H

#include <SDL2/SDL.h>

#include "main.h"

void allocatePixels();
void freePixels();

SDL_Texture* mapMandelbrotSet(SDL_Renderer* ren);
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

extern view_s view;

#endif //MANDELBROT_MAPPER_MANDELBROT_H