//
// Created by clovis on 12/27/23.
//

#include "mandelbrot.h"
#include "main.h"

#include <math.h>

#define MAX_ITERATIONS 100

// If a number squared MAX_ITERATIONS times is greater than UNSTABLE_THRESHOLD, it is considered unstable
#define UNSTABLE_THRESHOLD 100000

static int isStable(double x);
static DPoint convertScreenPointToMandelbrotPoint(SDL_Point screenPoint);
static int isPartOfMandelbrotSet(double re, double im);

SDL_Texture* mapMandelbrotSet(SDL_Renderer* ren)
{
    SDL_Texture* mandelbrotTexture = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888,
                                                       SDL_TEXTUREACCESS_TARGET, RESOLUTION_X, RESOLUTION_Y);

    if (mandelbrotTexture == NULL)
    {
        printf("Error creating texture: %s\n", SDL_GetError());
    }

    SDL_SetRenderTarget(ren, mandelbrotTexture);

    SDL_Point currentPixel = {0, 0};
    DPoint mandelbrotCoords = {0, 0};

    for (currentPixel.x = 0; currentPixel.x < RESOLUTION_X; currentPixel.x++)
    {
        for (currentPixel.y = 0; currentPixel.y < RESOLUTION_X; currentPixel.y++)
        {
            mandelbrotCoords = convertScreenPointToMandelbrotPoint(currentPixel);
            if (isPartOfMandelbrotSet(mandelbrotCoords.x, mandelbrotCoords.y))
            {
                SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            }
            else
            {
                SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
            }
            SDL_RenderDrawPoint(ren, currentPixel.x, currentPixel.y);
        }
    }

    SDL_SetRenderTarget(ren, NULL);
    return mandelbrotTexture;
}

void test1()
{
    SDL_Point screenPoint = {320, 0};
    scanf("%d", &screenPoint.y);
    DPoint result = convertScreenPointToMandelbrotPoint(screenPoint);
    printf("x: %f, y: %f\n", result.x, result.y);

}

static DPoint convertScreenPointToMandelbrotPoint(SDL_Point screenPoint)
{
    DPoint mandelbrotPoint;

    mandelbrotPoint.x = (double)screenPoint.x / LAST_X_PIXEL * 4 - 2;

    double yRange = (double)RESOLUTION_Y / RESOLUTION_X;

    mandelbrotPoint.y = (double)screenPoint.y / LAST_Y_PIXEL * 4 * yRange - 2 * yRange;

    return mandelbrotPoint;
}

static int isPartOfMandelbrotSet(double re, double im)
{
    double zReal = 0.0;
    double zImag = 0.0;

    for (int i = 0; i < MAX_ITERATIONS; ++i) {
        double zRealTemp = zReal * zReal - zImag * zImag + re;
        double zImagTemp = 2 * zReal * zImag + im;

        zReal = zRealTemp;
        zImag = zImagTemp;

        if (!isStable(zReal) || !isStable(zImag))
        {
            return 0; // Point not in Mandelbrot set
        }

        // If the magnitude of z becomes too large, consider it unstable
        if (sqrt(zReal * zReal + zImag * zImag) > MAX_ITERATIONS)
        {
            return 0; // Point not in Mandelbrot set
        }
    }

    return 1; // Point in Mandelbrot set
}

// Checks if the number passed is stable when squaring it MAX_ITERATIONS times
static int isStable(double x)
{
    // square x MAX_ITERATIONS times
    for (int i = 0; i < MAX_ITERATIONS; i++)
    {
        x *= x;
    }

    if (x > UNSTABLE_THRESHOLD)
    {
        // x is unstable
        return 0;
    }
    else
    {
        // x is stable
        return 1;
    }
}