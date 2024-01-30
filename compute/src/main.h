//
// Created by clovis on 12/27/23.
//

#ifndef MANDELBROT_MAPPER_MAIN_H
#define MANDELBROT_MAPPER_MAIN_H

#define RESOLUTION_X 640
#define RESOLUTION_Y 480
#define WINDOW_FLAGS SDL_WINDOW_SHOWN

typedef struct
{
    double x;
    double y;
} DPoint;

typedef struct
{
    int x;
    int y;
} IPoint;

extern int quit;

#define LAST_X_PIXEL (RESOLUTION_X - 1)
#define LAST_Y_PIXEL (RESOLUTION_Y - 1)

#endif //MANDELBROT_MAPPER_MAIN_H
