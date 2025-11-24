#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#define WINDOW_TITLE "Muskrat"
#define SCREEN_WIDTH 300
#define SCREEN_HEIGHT 300

#define GAME_DURATION_MS 10000   // 10 seconds
#define BUTTON_WIDTH 64
#define BUTTON_HEIGHT 64
#define BUTTON_LIFETIME 800   

typedef enum {
    STATE_INSTRUCTION,
    STATE_GAME,
    STATE_END
} GameState;

static GameState game_state = STATE_INSTRUCTION;

static SDL_Texture *background_tex = NULL;
static SDL_Texture *instruction_tex = NULL;
static SDL_Texture *button_tex = NULL;
static SDL_Texture *rod_up_tex = NULL;
static SDL_Texture *rod_down_tex = NULL;
static SDL_Texture *win_tex = NULL;
static SDL_Texture *lose_tex = NULL;

static int score = 0;
static Uint32 game_start_time = 0;
static bool game_running = false;
static bool instruction_visible = true;
static bool player_won = false;
static bool player_lost = false;

typedef struct {
    SDL_Rect rect;
    SDL_Texture *texture;
    bool isVisible;
    Uint32 spawnTime;
} Button;

static Button fishButton = {0};

void spawn_fish_button(SDL_Renderer *renderer);
void show_end_screen(SDL_Renderer *renderer);

static int rand_range(int min, int max) {
    if (max <= min) return min;
    return (rand() % (max - min + 1)) + min;
}

bool load_assets(SDL_Renderer *renderer) {

    background_tex = IMG_LoadTexture(renderer, "assets/FishingSea.png");

    instruction_tex = IMG_LoadTexture(renderer, "assets/FishInstruction.png");

    button_tex = IMG_LoadTexture(renderer, "assets/button.png");

    rod_up_tex = IMG_LoadTexture(renderer, "assets/FishingRod_up.png");

    rod_down_tex = IMG_LoadTexture(renderer, "assets/FishingRod_down.png");

    win_tex = IMG_LoadTexture(renderer, "assets/FishingWin.png");

    lose_tex = rod_up_tex;

    fishButton.isVisible = false;
    fishButton.texture = NULL;
    fishButton.rect.w = BUTTON_WIDTH;
    fishButton.rect.h = BUTTON_HEIGHT;
    fishButton.spawnTime = 0;

    return true;
}

void unload_assets(void) {
    if (background_tex) { SDL_DestroyTexture(background_tex); background_tex = NULL; }
    if (instruction_tex) { SDL_DestroyTexture(instruction_tex); instruction_tex = NULL; }
    if (button_tex) { SDL_DestroyTexture(button_tex); button_tex = NULL; }
    if (rod_up_tex) { SDL_DestroyTexture(rod_up_tex); rod_up_tex = NULL; }
    if (rod_down_tex) { SDL_DestroyTexture(rod_down_tex); rod_down_tex = NULL; }
    if (win_tex) { SDL_DestroyTexture(win_tex); win_tex = NULL; }
}

void spawn_fish_button(SDL_Renderer *renderer) {
    fishButton.texture = button_tex;

    int w = BUTTON_WIDTH;
    int h = BUTTON_HEIGHT;

    int x = rand_range(0, SCREEN_WIDTH - w);
    int y = rand_range(0, SCREEN_HEIGHT - h);

    fishButton.rect.x = x;
    fishButton.rect.y = y;
    fishButton.rect.w = w;
    fishButton.rect.h = h;

    fishButton.isVisible = true;
    fishButton.spawnTime = SDL_GetTicks();
}

void hide_fish_button(void) {
    fishButton.isVisible = false;
    fishButton.spawnTime = 0;
}

void show_end_screen(SDL_Renderer *renderer) {

    SDL_RenderCopy(renderer, background_tex, NULL, NULL);

    SDL_Texture *end_tex = player_won ? win_tex : lose_tex;
    if (end_tex) {
        SDL_Rect full = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_RenderCopy(renderer, end_tex, NULL, &full);
    }

    SDL_RenderPresent(renderer);

    SDL_Event e;
    bool waiting = true;
    while (waiting) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { waiting = false; break; }
            if (e.type == SDL_KEYDOWN || e.type == SDL_MOUSEBUTTONDOWN) { waiting = false; break; }
        }
        SDL_Delay(8);
    }
}

void handle_game_click(int mx, int my) {
    if (game_state == STATE_INSTRUCTION) {
        instruction_visible = false;
        game_state = STATE_GAME;
        game_running = true;
        score = 0;
        game_start_time = SDL_GetTicks();
        spawn_fish_button(NULL);
        return;
    }

    if (game_state != STATE_GAME) return;

    if (fishButton.isVisible) {
        if (mx >= fishButton.rect.x && mx <= fishButton.rect.x + fishButton.rect.w &&
            my >= fishButton.rect.y && my <= fishButton.rect.y + fishButton.rect.h) {
            score++;
            hide_fish_button();
            if (score >= 10) {
                player_won = true;
                game_running = false;
                game_state = STATE_END;
            }
        }
    }
}

void update_game(SDL_Renderer *renderer) {
    Uint32 now = SDL_GetTicks();

    if (game_state == STATE_GAME) {
        if (game_running && (now - game_start_time >= GAME_DURATION_MS)) {
            game_running = false;
            if (score >= 10) {
                player_won = true;
            } else {
                player_lost = true;
            }
            game_state = STATE_END;
            return;
        }

        if (fishButton.isVisible && (now - fishButton.spawnTime >= BUTTON_LIFETIME)) {
            hide_fish_button();
            player_lost = true;
            game_running = false;
            game_state = STATE_END;
            return;
        }

        if (game_running && !fishButton.isVisible) {
            spawn_fish_button(renderer);
        }
    }
}

void render_game(SDL_Renderer *renderer) {
    SDL_RenderClear(renderer);

    SDL_RenderCopy(renderer, background_tex, NULL, NULL);

    SDL_Rect rod_rect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};

    if (game_state == STATE_INSTRUCTION || game_state == STATE_END)
    {
        SDL_RenderCopy(renderer, rod_up_tex, NULL, &rod_rect);
    }
    else if (game_state == STATE_GAME)
    {
        SDL_RenderCopy(renderer, rod_down_tex, NULL, &rod_rect);
    }

    if (game_state == STATE_INSTRUCTION) {
        SDL_Rect r = { (SCREEN_WIDTH - 240) / 2, (SCREEN_HEIGHT - 160) / 2, 240, 160 };
        SDL_RenderCopy(renderer, instruction_tex, NULL, &r);
        SDL_RenderPresent(renderer);
        return;
    }

    if (game_state == STATE_GAME) {
        if (fishButton.isVisible && fishButton.texture) {
            SDL_RenderCopy(renderer, fishButton.texture, NULL, &fishButton.rect);
        }

        SDL_RenderPresent(renderer);
        return;
    }

    if (game_state == STATE_END) {
        SDL_RenderPresent(renderer);
        return;
    }
}


int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        fprintf(stderr, "IMG_Init error: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (!window) {
        fprintf(stderr, "CreateWindow error: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "CreateRenderer error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    srand((unsigned)time(NULL));

    if (!load_assets(renderer)) {
        fprintf(stderr, "Failed to load assets\n");
        unload_assets();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Event e;
    bool running = true;

    while (running) {

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { running = false; break; }
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) { running = false; break; }
                if (game_state == STATE_INSTRUCTION) {
              
                    instruction_visible = false;
                    game_state = STATE_GAME;
                    game_running = true;
                    score = 0;
                    game_start_time = SDL_GetTicks();
                    spawn_fish_button(renderer);
                }
            }
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                int mx = e.button.x;
                int my = e.button.y;
                if (game_state == STATE_INSTRUCTION) {
                    instruction_visible = false;
                    game_state = STATE_GAME;
                    game_running = true;
                    score = 0;
                    game_start_time = SDL_GetTicks();
                    spawn_fish_button(renderer);
                } else if (game_state == STATE_GAME) {
                    handle_game_click(mx, my);
                } else if (game_state == STATE_END) {
              
                    running = false;
                }
            }
        }

        update_game(renderer);

        render_game(renderer);

        if (game_state == STATE_END) {
            show_end_screen(renderer);
            break;
        }

        SDL_Delay(16); // ~60 FPS
    }

    unload_assets();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
