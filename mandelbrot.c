//
// Created by clovis on 12/27/23.
//

#include "mandelbrot.h"
#include "main.h"

#include <math.h>

#define MAX_ITERATIONS 256

// If a number squared MAX_ITERATIONS times is greater than UNSTABLE_THRESHOLD, it is considered unstable
#define UNSTABLE_THRESHOLD 100000

// I didn't really think this through, this is a trial and error thing.
// Funny things happen to the colors when this number is changed.
#define FUNNY_NUMBER 300000

static int isStable(double x);
static DPoint convertScreenPointToMandelbrotPoint(SDL_Point screenPoint);
static Uint32 isOutsideOfMandelbrotSet(double re, double im);
static SDL_Color hexToSDLColor(Uint32 hexValue);

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

    SDL_Color color = {0, 0, 0, 255};

    for (currentPixel.x = 0; currentPixel.x < RESOLUTION_X; currentPixel.x++)
    {
        for (currentPixel.y = 0; currentPixel.y < RESOLUTION_X; currentPixel.y++)
        {
            mandelbrotCoords = convertScreenPointToMandelbrotPoint(currentPixel);

            color = hexToSDLColor(FUNNY_NUMBER * isOutsideOfMandelbrotSet(mandelbrotCoords.x, mandelbrotCoords.y));

            SDL_SetRenderDrawColor(ren, color.r, color.g, color.b, color.a);

            SDL_RenderDrawPoint(ren, currentPixel.x, currentPixel.y);
        }
    }

    SDL_SetRenderTarget(ren, NULL);
    return mandelbrotTexture;
}

static DPoint convertScreenPointToMandelbrotPoint(SDL_Point screenPoint)
{
    DPoint mandelbrotPoint;

    mandelbrotPoint.x = (double)screenPoint.x / LAST_X_PIXEL * 4 - 2;

    double yRange = (double)RESOLUTION_Y / RESOLUTION_X;

    mandelbrotPoint.y = (double)screenPoint.y / LAST_Y_PIXEL * 4 * yRange - 2 * yRange;

    return mandelbrotPoint;
}

static Uint32 isOutsideOfMandelbrotSet(double re, double im)
{
    double zReal = 0.0;
    double zImag = 0.0;

    for (int i = 0; i < MAX_ITERATIONS; ++i) {
        double zRealTemp = zReal * zReal - zImag * zImag + re;
        double zImagTemp = 2 * zReal * zImag + im;

        zReal = zRealTemp;
        zImag = zImagTemp;

        // If the magnitude of z becomes too large, consider it unstable
        if (sqrt(zReal * zReal + zImag * zImag) > UNSTABLE_THRESHOLD)
        {
            return i; // Point not in Mandelbrot set
        }
    }

    return 0; // Point in Mandelbrot set
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