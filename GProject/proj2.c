#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>

typedef struct
{
    int x, y;
    short life;
    char *name;
} Man;

typedef struct 
{
    Man man;
} GameState;

int processEvents(SDL_Window *window, GameState *game)
{
    SDL_Event event;
    int done = 0;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_WINDOWEVENT_CLOSE:
                if (window)
                {
                    SDL_DestroyWindow(window);
                    window = NULL;
                    done = 1;
                }
                break;

            case SDL_KEYDOWN: 
                switch (event.key.keysym.sym)
                {
                    case SDLK_ESCAPE:
                        done = 1;
                        break;
                }
                break;

            case SDL_QUIT:
                done = 1;
                break;
        }
    }

    const Uint8 *state = SDL_GetKeyboardState(NULL);
    if (state[SDL_SCANCODE_LEFT])
        game->man.x -= 10;

    if (state[SDL_SCANCODE_RIGHT])
        game->man.x += 10; 

    if (state[SDL_SCANCODE_UP])
        game->man.y -= 10;

    if (state[SDL_SCANCODE_DOWN])
        game->man.y += 10;

    return done;
}

void doRender(SDL_Renderer *renderer, GameState *game) 
{
    // blue wallpaper
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    SDL_RenderClear(renderer);

    // white squer
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect rect = { game->man.x, game->man.y, 80, 80 };
    SDL_RenderFillRect(renderer, &rect);

    SDL_RenderPresent(renderer);
}

int main(int argc, char *argv[])
{
    GameState gameState;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    SDL_Init(SDL_INIT_VIDEO);

    gameState.man.x = 320 - 40;
    gameState.man.y = 240 - 40;

    window = SDL_CreateWindow(
        "Game Window",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        640,
        480,
        0
    );

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    int done = 0;
    while (!done)
    {
        done = processEvents(window, &gameState);
        doRender(renderer, &gameState);
        SDL_Delay(16); 
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
