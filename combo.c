#define SDL_MAIN_HANDLED

#include <SDL.h>
#include <SDL_image.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define WINDOW_TITLE "Muskrat"
#define SCREEN_WIDTH 350
#define SCREEN_HEIGHT 350
#define IMAGE_FLAGS IMG_INIT_PNG

typedef enum {
    STATE_MENU,
    STATE_GAME
} GameState;

struct Game {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *background;
    SDL_Texture *character;
    SDL_Texture *menu;
};

void game_cleanup(struct Game *game, int exit_status);
bool load_media(struct Game *game) {

    game->menu = IMG_LoadTexture(game->renderer, "assets/menu.png");
    if (!game->menu) {
        fprintf(stderr, "Error loading menu: %s\n", IMG_GetError());
        SDL_Delay(3000);
        return true;
    }
    
    game->background = IMG_LoadTexture(game->renderer, "assets/background.png"); // file with background
    if (!game->background) {
        fprintf(stderr, "Error loading background: %s\n", IMG_GetError());
        SDL_Delay(3000);
        return true;
    }

    game->character = IMG_LoadTexture(game->renderer, "assets/character.png"); // file with character
    if (!game->character) {
        fprintf(stderr, "Error loading character: %s\n", IMG_GetError()); 
        SDL_Delay(3000);
        return true;
    }

    return false;
}

bool sdl_initialize(struct Game *game);

int main(void) {
    struct Game game = {0};

    if (sdl_initialize(&game) || load_media(&game)) {
    game_cleanup(&game, EXIT_FAILURE);
}


    bool running = true;
    GameState state = STATE_MENU;

    while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            running = false;
            break;

        case SDL_KEYDOWN:
            if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                if (state == STATE_MENU)
                    running = false;  // exit
                else
                    state = STATE_MENU; // comeback to menu
            }
            else if (event.key.keysym.scancode == SDL_SCANCODE_RETURN) {
                if (state == STATE_MENU)
                    state = STATE_GAME; // start
            }
            break;
        }
    }

    SDL_RenderClear(game.renderer);

    if (state == STATE_MENU) {
        SDL_RenderCopy(game.renderer, game.menu, NULL, NULL);
    } 
    else if (state == STATE_GAME) {
        SDL_RenderCopy(game.renderer, game.background, NULL, NULL);

        int char_width = 0, char_height = 0;
        SDL_QueryTexture(game.character, NULL, NULL, &char_width, &char_height);

        SDL_Rect dest = {
            (SCREEN_WIDTH - char_width) / 2,
            (SCREEN_HEIGHT - char_height) / 2 + 60,
            char_width,
            char_height
        };

        SDL_RenderCopy(game.renderer, game.character, NULL, &dest);
    }

    SDL_RenderPresent(game.renderer);
    SDL_Delay(16); // ~60 FPS
}


    game_cleanup(&game, EXIT_SUCCESS);
    return 0;
}

void game_cleanup(struct Game *game, int exit_status) {
    if (game->menu) SDL_DestroyTexture(game->menu);  
    if (game->character) SDL_DestroyTexture(game->character);
    if (game->background) SDL_DestroyTexture(game->background);
    if (game->renderer) SDL_DestroyRenderer(game->renderer);
    if (game->window) SDL_DestroyWindow(game->window);
    IMG_Quit();
    SDL_Quit();
    exit(exit_status);
}

bool sdl_initialize(struct Game *game) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Error initializing SDL: %s\n", SDL_GetError());
        return true;
    }

    int img_init = IMG_Init(IMAGE_FLAGS);
    if ((img_init & IMAGE_FLAGS) != IMAGE_FLAGS) {
        fprintf(stderr, "Error initializing SDL_image: %s\n", IMG_GetError());
        return true;
    }

    game->window = SDL_CreateWindow(
        WINDOW_TITLE,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    if (!game->window) {
        fprintf(stderr, "Error creating window: %s\n", SDL_GetError());
        return true;
    }

    game->renderer = SDL_CreateRenderer(game->window, -1, SDL_RENDERER_ACCELERATED);
    if (!game->renderer) {
        fprintf(stderr, "Error creating renderer: %s\n", SDL_GetError());
        return true;
    }

    return false;
}

