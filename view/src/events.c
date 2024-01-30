//
// Created by clovis on 12/30/23.
//

#include "main.h"
#include "mandelbrot.h"

int quit = 0;
SDL_Point mouse;

const Uint8* keyStates;

int shouldPerformKeyAction(int scancode);

int handleEvents()
{
    static SDL_Event event;

    keyStates = SDL_GetKeyboardState(NULL);

    double aspectRatio = (double)RESOLUTION_X / RESOLUTION_Y;

    while (SDL_WaitEvent(&event) && !quit)
    {
        if (event.type == SDL_QUIT || event.window.event == SDL_WINDOWEVENT_CLOSE)
        {
            quit = 1;
        }

        SDL_GetMouseState(&mouse.x, &mouse.y);

        if (shouldPerformKeyAction(SDL_SCANCODE_EQUALS))
        {
            zoomInViewSize();
        }

        if (shouldPerformKeyAction(SDL_SCANCODE_MINUS))
        {
            zoomOutViewSize();
        }


        if (shouldPerformKeyAction(SDL_SCANCODE_RIGHT))
        {
            view.centerPoint.x += 50 / (RESOLUTION_X / view.viewWidth);
        }

        if (shouldPerformKeyAction(SDL_SCANCODE_LEFT))
        {
            view.centerPoint.x -= 50 / (RESOLUTION_X / view.viewWidth);
        }

        if (shouldPerformKeyAction(SDL_SCANCODE_UP))
        {
            view.centerPoint.y -= 50 / (RESOLUTION_Y / view.viewHeight);
        }

        if (shouldPerformKeyAction(SDL_SCANCODE_DOWN))
        {
            view.centerPoint.y += 50 / (RESOLUTION_Y / view.viewHeight);
        }

        if (event.key.keysym.sym == SDLK_ESCAPE)
        {
            quit = 1;
        }
    }

    return 0;
}

// This ensures an action is performed once and doesn't repeat when a key is pressed. To perform the same action
// again, the key must be released and pressed again.
int shouldPerformKeyAction(int scancode)
{
    static int lockKey[SDL_NUM_SCANCODES] = {0};

    if (keyStates[scancode] && !lockKey[scancode])
    {
        lockKey[scancode] = 1;
        return 1;
    }
    else if (!keyStates[scancode])
    {
        lockKey[scancode] = 0;
        return 0;
    }
}