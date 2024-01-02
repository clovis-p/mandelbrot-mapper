//
// Created by clovis on 12/27/23.
//

#include "mandelbrot.h"
#include "main.h"

#include <math.h>
#include <time.h>

// If a number squared MAX_ITERATIONS times is greater than UNSTABLE_THRESHOLD, it is considered unstable
#define UNSTABLE_THRESHOLD 1000000

// I didn't really think this through, this is a trial and error thing.
// Funny things happen to the colors when this number is changed.
#define FUNNY_NUMBER (-985432)

// 1 = use the number defined above
// 0 = use a random number
#define USE_FUNNY_NUMBER 0

static int isStable(double x);
static DPoint convertScreenPointToMandelbrotPoint(DPoint screenPoint, double viewWidth, double viewHeight, DPoint centerPoint);
static Uint32 isOutsideOfMandelbrotSet(double re, double im);
static SDL_Color assignColorToMandelbrotPoint(DPoint point);
static SDL_Color hexToSDLColor(Uint32 hexValue);

view_s view = {RESOLUTION_X, RESOLUTION_Y,
               ((double)LAST_X_PIXEL / 2), ((double)LAST_Y_PIXEL / 2)};

int maxIterations = 512;

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
    DPoint currentPixelD = {0, 0,};
    DPoint mandelbrotCoords = {0, 0};

    SDL_Color color = {0, 0, 0, 255};

    int funnyNumber;

    if (!USE_FUNNY_NUMBER)
    {
        srand(time(NULL));
        funnyNumber = rand() % 1000000000 - 500000000;
        printf("Funny number = %d\n", funnyNumber);
    }
    else
    {
        funnyNumber = FUNNY_NUMBER;
    }

    for (currentPixel.x = 0; currentPixel.x < RESOLUTION_X; currentPixel.x++)
    {
        for (currentPixel.y = 0; currentPixel.y < RESOLUTION_X; currentPixel.y++)
        {
            currentPixelD.x = (double)currentPixel.x;
            currentPixelD.y = (double)currentPixel.y;
            DPoint topLeftCorner = {(double)RESOLUTION_X / 2 + 190,(double)RESOLUTION_Y / 2 + 35};
            mandelbrotCoords = convertScreenPointToMandelbrotPoint(currentPixelD, view.viewWidth, view.viewHeight, view.centerPoint);

            color = assignColorToMandelbrotPoint(mandelbrotCoords);

            SDL_SetRenderDrawColor(ren, color.r, color.g, color.b, color.a);

            SDL_RenderDrawPoint(ren, currentPixel.x, currentPixel.y);
        }
    }

    SDL_SetRenderTarget(ren, NULL);
    return mandelbrotTexture;
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
        if (sqrt(zReal * zReal + zImag * zImag) > UNSTABLE_THRESHOLD)
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
    Uint32 hexValue = FUNNY_NUMBER * isOutsideOfMandelbrotSet(point.x, point.y) * 256 / maxIterations;

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