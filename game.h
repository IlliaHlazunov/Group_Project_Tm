#ifndef GAME_H
#define GAME_H

#include "game_state.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

GameState run_game(SDL_Renderer *renderer);

#endif