//
// Created by clovis on 12/27/23.
//

#ifndef MANDELBROT_MAPPER_MANDELBROT_H
#define MANDELBROT_MAPPER_MANDELBROT_H

#include <SDL2/SDL.h>

SDL_Texture* mapMandelbrotSet(SDL_Renderer* ren);

typedef struct
{
    double real;
    double imaginary;
} complex_s;

typedef struct
{
    double x;
    double y;
} DPoint;

#endif //MANDELBROT_MAPPER_MANDELBROT_H
