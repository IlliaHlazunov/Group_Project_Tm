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

#define GAME_DURATION_MS 15000   // 15 seconds
#define BUTTON_WIDTH 64
#define BUTTON_HEIGHT 64
#define BUTTON_LIFETIME 800
#define EXIT_WIDTH 32
#define EXIT_HEIGHT 32

typedef enum {
    STATE_INSTRUCTION,
    STATE_GAME,
    STATE_END
} GameState;

static GameState game_state = STATE_INSTRUCTION;

// texture
static SDL_Texture *background_tex = NULL;
static SDL_Texture *instruction_tex = NULL;
static SDL_Texture *button_tex = NULL;
static SDL_Texture *exit_tex = NULL;
static SDL_Texture *rod_up_tex = NULL;
static SDL_Texture *rod_down_tex = NULL;
static SDL_Texture *win_tex = NULL;
static SDL_Texture *lose_tex = NULL;

// game state variables
static int score = 0; // Player score
static Uint32 game_start_time = 0; // Time when the game started
static bool game_running = false;  // Is the game currently active
static bool player_won = false;    // Did the player win
static bool player_lost = false;   // Did the player lose

// animation variables
static int frame = 0;
static Uint32 lastTime = 0; 
static int frameWidth = 0;
static int frameHeight = 0;

typedef struct {
    SDL_Rect rect;
    SDL_Texture *texture;
    bool isVisible;
  	Uint32 spawnTime;
} Button;

static Button fishButton = {0};
static SDL_Rect exit_rect = { 8, 8, EXIT_WIDTH, EXIT_HEIGHT };

static int rand_range(int min, int max) {
    if (max <= min) return min;
    return (rand() % (max - min + 1)) + min;
}

// Resets the game state to default/start screen
void reset_game(void) {
    score = 0;
    player_won = false;
    player_lost = false;
    game_running = false;
    game_state = STATE_INSTRUCTION; // Back to instructions
    fishButton.isVisible = false;
    fishButton.spawnTime = 0;
}

// Load textures from files
bool load_assets(SDL_Renderer *renderer) {
    background_tex = IMG_LoadTexture(renderer, "assets/FishingSea.png");
    instruction_tex = IMG_LoadTexture(renderer, "assets/FishInstruction.png");
    button_tex = IMG_LoadTexture(renderer, "assets/button.png");
    exit_tex = IMG_LoadTexture(renderer, "assets/exit.png");
    rod_up_tex = IMG_LoadTexture(renderer, "assets/FishingRod_up.png");
    rod_down_tex = IMG_LoadTexture(renderer, "assets/FishingRod_down.png");
    win_tex = IMG_LoadTexture(renderer, "assets/FishingWin.png");
    lose_tex = rod_up_tex; // Use rod_up_tex for the lose screen

    // Initialize button parameters
    fishButton.isVisible = false;
    fishButton.texture = button_tex;
    fishButton.rect.w = BUTTON_WIDTH;
    fishButton.rect.h = BUTTON_HEIGHT;
    fishButton.spawnTime = 0;

    int w, h; // animation for instruction (2 frames)
    SDL_QueryTexture(instruction_tex, NULL, NULL, &w, &h);

    frameWidth = w / 2;  
    frameHeight = h;

    frame = 0;
    lastTime = SDL_GetTicks();
    
    return true;
}

// unloads and destroys all textures
void unload_assets(void) {
    if (background_tex) { SDL_DestroyTexture(background_tex); background_tex = NULL; }
    if (instruction_tex) { SDL_DestroyTexture(instruction_tex); instruction_tex = NULL; }
    if (button_tex) { SDL_DestroyTexture(button_tex); button_tex = NULL; }
    if (exit_tex) { SDL_DestroyTexture(exit_tex); exit_tex = NULL; }
    if (rod_up_tex) { SDL_DestroyTexture(rod_up_tex); rod_up_tex = NULL; }
    if (rod_down_tex) { SDL_DestroyTexture(rod_down_tex); rod_down_tex = NULL; }
    if (win_tex) { SDL_DestroyTexture(win_tex); win_tex = NULL; }
}

// spawns button at a random location
void spawn_fish_button(SDL_Renderer *renderer) {
    (void)renderer;
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

// hides button
void hide_fish_button(void) {
    fishButton.isVisible = false;
    fishButton.spawnTime = 0;
}

bool show_end_screen(SDL_Renderer *renderer) {
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, background_tex, NULL, NULL);
    // Choose win or lose texture
    SDL_Texture *end_tex = player_won ? win_tex : lose_tex;
    if (end_tex) {
        SDL_Rect full = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_RenderCopy(renderer, end_tex, NULL, &full);
    }
    SDL_RenderPresent(renderer);

    SDL_Event e;
    while (1) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) return true;
            if (e.type == SDL_KEYDOWN || e.type == SDL_MOUSEBUTTONDOWN) return false;
        }
        SDL_Delay(8);
    }
}

void handle_game_click(int mx, int my) {
    if (game_state != STATE_GAME) return;

    // Check if the click is within button
    if (fishButton.isVisible) {
        if (mx >= fishButton.rect.x && mx <= fishButton.rect.x + fishButton.rect.w &&
            my >= fishButton.rect.y && my <= fishButton.rect.y + fishButton.rect.h) {
            score++;
            hide_fish_button();
            if (score >= 10) { // winning condition
                player_won = true;
                int FishLeft = 0;
                FILE *fishFile = fopen("FishCount.txt", "r");
                if (fishFile != NULL) {
                    fscanf(fishFile, "%d", &FishLeft);
                    fclose(fishFile);
                }
                if (FishLeft < 4){
                    FILE *file = fopen("FishCount.txt", "w");
                    if (file != NULL) {
                        fprintf(file, "%d", FishLeft + 1);
                        fclose(file);
                    }
                }

                game_running = false;
                game_state = STATE_END;
            }
        }
    }
}

void update_game(void) {
    Uint32 now = SDL_GetTicks();

    if (game_state == STATE_GAME) {
        if (game_running && (now - game_start_time >= GAME_DURATION_MS)) {
            game_running = false;
            if (score >= 10) player_won = true;
            else player_lost = true;
            game_state = STATE_END;
            return;
        }

        // check button lifetime
        if (fishButton.isVisible && (now - fishButton.spawnTime >= BUTTON_LIFETIME)) {
            hide_fish_button();
            player_lost = true;
            game_running = false;
            game_state = STATE_END;
            return;
        }

        // spawn new button if current hiden
        if (game_running && !fishButton.isVisible) {
            spawn_fish_button(NULL);
        }
    }
}

void render_game(SDL_Renderer *renderer) {
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, background_tex, NULL, NULL);

    SDL_Rect rod_rect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    if (game_state == STATE_INSTRUCTION || game_state == STATE_END) {
        SDL_RenderCopy(renderer, rod_up_tex, NULL, &rod_rect);
    } else if (game_state == STATE_GAME) {
        SDL_RenderCopy(renderer, rod_down_tex, NULL, &rod_rect);
    }

    if (game_state == STATE_INSTRUCTION) {

    	// animation instruction
       Uint32 now = SDL_GetTicks();
    if (now - lastTime >= 400) { //0.4 sec
        frame = (frame + 1) % 2;
        lastTime = now;
    }

    SDL_Rect src = { frame * frameWidth, 0, frameWidth, frameHeight };

    SDL_Rect dst = {
        (SCREEN_WIDTH - 250) / 2, (SCREEN_HEIGHT - 250) / 2, 250, 250
    };

    SDL_RenderCopy(renderer, instruction_tex, &src, &dst);
    SDL_RenderCopy(renderer, exit_tex, NULL, &exit_rect);
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

    // create window
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

    reset_game();

    SDL_Event e; // main loop
    bool running = true;

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { running = false; break; }

            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) { running = false; break; }

                if (game_state == STATE_INSTRUCTION) {
                    game_state = STATE_GAME;
                    game_running = true;
                    score = 0;
                    game_start_time = SDL_GetTicks();
                    spawn_fish_button(renderer);
                } else if (game_state == STATE_END) {
                    reset_game();
                }
            }

            if (e.type == SDL_MOUSEBUTTONDOWN) {
                int mx = e.button.x;
                int my = e.button.y;

                if (game_state == STATE_INSTRUCTION) {
                    if (mx >= exit_rect.x && mx <= exit_rect.x + exit_rect.w &&
                        my >= exit_rect.y && my <= exit_rect.y + exit_rect.h) {
                        running = false;
                        system("start Main_Screen.exe");
                        system("taskkill /F /IM FishingGame.exe");
                    } else {
                        game_state = STATE_GAME;
                        game_running = true;
                        score = 0;
                        game_start_time = SDL_GetTicks();
                        spawn_fish_button(renderer);
                    }
                } else if (game_state == STATE_GAME) {
                    handle_game_click(mx, my);
                } else if (game_state == STATE_END) {
                    reset_game();
                }
            }
        }

        update_game();

        render_game(renderer);

        if (game_state == STATE_END) {
            bool quit = show_end_screen(renderer);
            if (quit) running = false;
            else reset_game();
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