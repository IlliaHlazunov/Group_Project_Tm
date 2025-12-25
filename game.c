#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_image.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define WINDOW_TITLE "Muskrat"
#define SCREEN_WIDTH 300
#define SCREEN_HEIGHT 300
#define IMAGE_FLAGS IMG_INIT_PNG

#define FRAME_DELAY_BG 100    
#define FRAMES_PER_SHEET 6
#define TOTAL_SHEETS 4
#define FRAME_DELAY_CHAR 150  

struct Game {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *backgroundSheets[TOTAL_SHEETS];
    SDL_Texture *character;
};

bool sdl_initialize(struct Game *game) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL init error: %s\n", SDL_GetError());
        return true;
    }
    if (!(IMG_Init(IMAGE_FLAGS) & IMAGE_FLAGS)) {
        fprintf(stderr, "SDL_image init error: %s\n", IMG_GetError());
        return true;
    }

    game->window = SDL_CreateWindow(WINDOW_TITLE,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!game->window) return true;

    game->renderer = SDL_CreateRenderer(game->window, -1, SDL_RENDERER_ACCELERATED);
    if (!game->renderer) return true;

    return false;
}

bool load_media(struct Game *game) {
    char filename[64];
    for (int i = 0; i < TOTAL_SHEETS; i++) {
        sprintf(filename, "assets/backgroundAn_%d.png", i + 1);
        game->backgroundSheets[i] = IMG_LoadTexture(game->renderer, filename);
        if (!game->backgroundSheets[i]) {
            fprintf(stderr, "Error loading %s: %s\n", filename, IMG_GetError());
            return true;
        }
    }

    game->character = IMG_LoadTexture(game->renderer, "assets/characterAn.png");
    if (!game->character) {
        fprintf(stderr, "Error loading character: %s\n", IMG_GetError());
        return true;
    }
    return false;
}

void game_cleanup(struct Game *game, int exit_status) {
    for (int i = 0; i < TOTAL_SHEETS; i++)
        if (game->backgroundSheets[i]) SDL_DestroyTexture(game->backgroundSheets[i]);
    if (game->character) SDL_DestroyTexture(game->character);
    if (game->renderer) SDL_DestroyRenderer(game->renderer);
    if (game->window) SDL_DestroyWindow(game->window);
    IMG_Quit();
    SDL_Quit();
    exit(exit_status);
}

int main(void) {
    struct Game game = {0};

    if (sdl_initialize(&game)) game_cleanup(&game, EXIT_FAILURE);
    if (load_media(&game)) game_cleanup(&game, EXIT_FAILURE);

    bool running = true;
    SDL_Event event;


    int currentSheet = 0, frameBG = 0;
    Uint32 lastFrameTimeBG = SDL_GetTicks();

    int texW, texH;
    SDL_QueryTexture(game.backgroundSheets[0], NULL, NULL, &texW, &texH);
    int frameHeightBG = texH / FRAMES_PER_SHEET;


    int frameChar = 0;
    Uint32 lastFrameTimeChar = 0;
    int total_frames_char = 4;
    int char_width = 0, char_height = 0;
    SDL_QueryTexture(game.character, NULL, NULL, &char_width, &char_height);
    int frame_width_char = char_width / total_frames_char;
    int frame_height_char = char_height;

    SDL_Rect destBG = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
                running = false;
        }

        Uint32 now = SDL_GetTicks();

        
        if (now - lastFrameTimeBG > FRAME_DELAY_BG) {
            frameBG++;
            if (frameBG >= FRAMES_PER_SHEET) {
                frameBG = 0;
                currentSheet++;
                if (currentSheet >= TOTAL_SHEETS) currentSheet = 0;
                SDL_QueryTexture(game.backgroundSheets[currentSheet], NULL, NULL, &texW, &texH);
                frameHeightBG = texH / FRAMES_PER_SHEET;
            }
            lastFrameTimeBG = now;
        }

        
        if (now > lastFrameTimeChar + FRAME_DELAY_CHAR) {
            frameChar = (frameChar + 1) % total_frames_char;
            lastFrameTimeChar = now;
        }

        
        SDL_Rect srcBG = {0, frameBG * frameHeightBG, texW, frameHeightBG};
        SDL_Rect srcChar = {frameChar * frame_width_char, 0, frame_width_char, frame_height_char};
        SDL_Rect destChar = {
            (SCREEN_WIDTH - frame_width_char) / 2,
            (SCREEN_HEIGHT - frame_height_char) / 2 + 40,
            frame_width_char,
            frame_height_char
        };

        
        SDL_RenderClear(game.renderer);
        SDL_RenderCopy(game.renderer, game.backgroundSheets[currentSheet], &srcBG, &destBG);
        SDL_RenderCopy(game.renderer, game.character, &srcChar, &destChar);
        SDL_RenderPresent(game.renderer);

        SDL_Delay(16);
    }

    game_cleanup(&game, EXIT_SUCCESS);
    return 0;
}
