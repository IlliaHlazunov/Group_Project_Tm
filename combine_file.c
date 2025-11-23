#define SDL_MAIN_HANDLED

#include <SDL.h>
#include "game_state.h"
#include "start.h"
#include "game.h"
#include "fish.h"

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Muskrat", 100, 100, 300, 300, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

    GameState state = STATE_START;

    while (state != STATE_EXIT) {
        switch (state) {
            case STATE_START:
                state = run_start(renderer);
                break;
            case STATE_GAME:
                state = run_game(renderer);
                break;
            case STATE_FISH:
                state = run_fish(renderer);
                break;
            default:
                state = STATE_EXIT;
                break;
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
