#define INITIAL_MAX_ITERATIONS 512
#define UNSTABLE_THRESHOLD 1000000
#define RESOLUTION_X 640
#define RESOLUTION_Y 480

#define FUNNY_NUMBER (-985432)

#define LAST_X_PIXEL (RESOLUTION_X - 1)
#define LAST_Y_PIXEL (RESOLUTION_Y - 1)

__kernel void computeMandelbrotPixel(__global const double* screenPoints, __global const double* viewParams, __global uint* colors)
{
    int i = get_global_id(0);

    double x = screenPoints[i * 2];
    double y = screenPoints[i * 2 + 1];

    double viewWidth = viewParams[0];
    double viewHeight = viewParams[1];
    double centerX = viewParams[2];
    double centerY = viewParams[3];

    double yRange = (double)RESOLUTION_Y / RESOLUTION_X;

    double mandelbrotX = x / LAST_X_PIXEL * (4 * (viewWidth / RESOLUTION_X)) - (2 * (viewWidth / RESOLUTION_X)) + (centerX / LAST_X_PIXEL * 4 - 2);
    double mandelbrotY = y / LAST_Y_PIXEL * (4 * (viewHeight / RESOLUTION_Y)) * yRange - (2 * (viewHeight / RESOLUTION_Y)) * yRange + (centerY / LAST_Y_PIXEL * 4 - 2);

    double zReal = 0.0;
    double zImag = 0.0;

    int maxIterations = INITIAL_MAX_ITERATIONS;
    int unstableThreshold = UNSTABLE_THRESHOLD;

    int isInsideOfMandelbrotSet = 0;

    int iterations;
    for (iterations = 0; iterations < maxIterations; ++iterations)
    {
        double zRealTemp = zReal * zReal - zImag * zImag + mandelbrotX;
        double zImagTemp = 2 * zReal * zImag + mandelbrotY;

        zReal = zRealTemp;
        zImag = zImagTemp;

        if (zReal * zReal + zImag * zImag > unstableThreshold)
        {
            isInsideOfMandelbrotSet = 1;
        }
    }

    if (isInsideOfMandelbrotSet)
    {
        uint hexValue = (FUNNY_NUMBER * iterations * 256 / INITIAL_MAX_ITERATIONS) % 0xFFFFFF;
        colors[i] = hexValue;
    }
    else
    {
        colors[i] = 0x000000;
    }
}