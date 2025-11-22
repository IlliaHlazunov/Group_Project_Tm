#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define WINDOW_WIDTH 300
#define WINDOW_HEIGHT 300

// Налаштування анімації
#define FRAME_DELAY_BG 100
#define FRAMES_PER_SHEET 6
#define TOTAL_SHEETS 4
#define FRAME_DELAY_CHAR 150
#define CHAR_FRAMES 4 

typedef enum { SCENE_MAP, SCENE_CAT } Scene;

typedef struct {
    int x, y;           
    int workTime;       
    int restTime;      
    SDL_Rect hitBox;    
} Flag;

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *map;
    SDL_Texture *backgroundSheets[TOTAL_SHEETS];
    SDL_Texture *character;
    TTF_Font *font;      
    TTF_Font *fontTimer; 
} Game;

Flag flags[3];

// === НОВА ЛОГІКА ГЕНЕРАЦІЇ (БЕЗ ПОВТОРІВ) ===
void randomizeFlags() {
    // Спочатку скидаємо значення, щоб не було сміття
    for (int i = 0; i < 3; i++) {
        flags[i].workTime = -1; 
    }

    for (int i = 0; i < 3; i++) {
        int val;
        bool isDuplicate;
        
        do {
            // Генеруємо число від 15 до 60 (крок 5)
            val = 15 + (rand() % 10) * 5;
            isDuplicate = false;

            // Перевіряємо, чи це число вже є в інших прапорцях
            for (int j = 0; j < i; j++) {
                if (flags[j].workTime == val) {
                    isDuplicate = true;
                    break; // Знайшли повтор, виходимо і крутимо rand знову
                }
            }
        } while (isDuplicate); // Крутимо цикл, поки не знайдемо унікальне

        flags[i].workTime = val;
        flags[i].restTime = val / 5;
    }
}

bool init(Game *game) {
    srand((unsigned int)time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO) < 0) return false;
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) return false;
    if (TTF_Init() < 0) return false;

    game->window = SDL_CreateWindow("Muskrat",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    
    game->renderer = SDL_CreateRenderer(game->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
    game->font = TTF_OpenFont("assets/OpenSans-Regular.ttf", 14);
    if (!game->font) game->font = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 14);
    
    game->fontTimer = TTF_OpenFont("assets/OpenSans-Regular.ttf", 20); 
    if (!game->fontTimer) game->fontTimer = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 20);

    return (game->renderer && game->font && game->fontTimer);
}

bool load_assets(Game *game) {
    game->map = IMG_LoadTexture(game->renderer, "assets/karta.png");
    
    char filename[64];
    for (int i = 0; i < TOTAL_SHEETS; i++) {
        sprintf(filename, "assets/backgroundAn_%d.png", i + 1);
        game->backgroundSheets[i] = IMG_LoadTexture(game->renderer, filename);
    }
    game->character = IMG_LoadTexture(game->renderer, "assets/characterAn.png");

    return (game->map && game->character && game->backgroundSheets[0]);
}

void cleanup(Game *game) {
    for (int i = 0; i < TOTAL_SHEETS; i++)
        if (game->backgroundSheets[i]) SDL_DestroyTexture(game->backgroundSheets[i]);
    if (game->character) SDL_DestroyTexture(game->character);
    if (game->map) SDL_DestroyTexture(game->map);
    if (game->renderer) SDL_DestroyRenderer(game->renderer);
    if (game->window) SDL_DestroyWindow(game->window);
    if (game->font) TTF_CloseFont(game->font);
    if (game->fontTimer) TTF_CloseFont(game->fontTimer);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

void renderText(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, text, color);
    if (!surf) return;
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_Rect rect = {x - surf->w / 2, y - surf->h / 2, surf->w, surf->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(texture);
}

int main() {
    Game game = {0};
    if (!init(&game)) return 1;
    if (!load_assets(&game)) { cleanup(&game); return 1; }

    flags[0] = (Flag){245, 50, 0, 0, {185, 30, 70, 45}};   
    flags[1] = (Flag){58, 230, 0, 0, {35, 200, 70, 45}};   
    flags[2] = (Flag){237, 240, 0, 0, {195, 210, 70, 45}}; 

    // Генеруємо унікальні цифри при старті
    randomizeFlags();

    Scene scene = SCENE_MAP;
    bool running = true;
    SDL_Event e;
    
    Uint32 lastFrameTimeBG = SDL_GetTicks();
    Uint32 lastFrameTimeChar = SDL_GetTicks();
    Uint32 timerStart = 0;
    
    int selectedTotalTime = 0; 
    int activeFlagIndex = -1;  
    bool isResting = false;    

    int currentSheet = 0, frameBG = 0, frameChar = 0;
    int bgW, bgH, charW, charH;

    SDL_QueryTexture(game.character, NULL, NULL, &charW, &charH);
    int frameWidthChar = charW / CHAR_FRAMES; 
    int frameHeightChar = charH;

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;

            if (scene == SCENE_MAP && e.type == SDL_MOUSEBUTTONDOWN) {
                SDL_Point mousePoint = {e.button.x, e.button.y};
                for (int i = 0; i < 3; i++) {
                    if (SDL_PointInRect(&mousePoint, &flags[i].hitBox)) {
                        scene = SCENE_CAT;
                        activeFlagIndex = i;
                        isResting = false; 
                        selectedTotalTime = flags[i].workTime * 60; 
                        timerStart = SDL_GetTicks();
                    }
                }
            }

            if (scene == SCENE_CAT && e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_MINUS || e.key.keysym.sym == SDLK_KP_MINUS) {
                    const Uint8 *state = SDL_GetKeyboardState(NULL);
                    if (state[SDL_SCANCODE_EQUALS] || state[SDL_SCANCODE_KP_PLUS]) {
                        timerStart -= 60 * 1000; 
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, 255);
        SDL_RenderClear(game.renderer);

        if (scene == SCENE_MAP) {
            SDL_RenderCopy(game.renderer, game.map, NULL, NULL);
            
            SDL_Color white = {255, 255, 255, 255};
            for (int i = 0; i < 3; i++) {
                char text[32];
                snprintf(text, sizeof(text), "%d / %d", flags[i].workTime, flags[i].restTime);
                renderText(game.renderer, game.font, text, flags[i].x, flags[i].y, white);
            }

        } else if (scene == SCENE_CAT) {
            int elapsed = (SDL_GetTicks() - timerStart) / 1000;
            int remaining = selectedTotalTime - elapsed;

            if (remaining <= 0) {
                remaining = 0;
                if (!isResting) {
                    isResting = true;
                    selectedTotalTime = flags[activeFlagIndex].restTime * 60;
                    timerStart = SDL_GetTicks(); 
                } else {
                    // Повернення на карту + оновлення цифр
                    scene = SCENE_MAP;
                    randomizeFlags(); 
                }
            }

            Uint32 now = SDL_GetTicks();
            if (now - lastFrameTimeBG > FRAME_DELAY_BG) {
                frameBG = (frameBG + 1) % FRAMES_PER_SHEET;
                if (frameBG == 0) currentSheet = (currentSheet + 1) % TOTAL_SHEETS;
                lastFrameTimeBG = now;
            }
            if (now - lastFrameTimeChar > FRAME_DELAY_CHAR) {
                frameChar = (frameChar + 1) % CHAR_FRAMES;
                lastFrameTimeChar = now;
            }

            SDL_QueryTexture(game.backgroundSheets[currentSheet], NULL, NULL, &bgW, &bgH);
            int frameHeightBG = bgH / FRAMES_PER_SHEET; 
            SDL_Rect srcBG = {0, frameBG * frameHeightBG, bgW, frameHeightBG};
            SDL_Rect destBG = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
            SDL_RenderCopy(game.renderer, game.backgroundSheets[currentSheet], &srcBG, &destBG);

            SDL_Rect srcChar = {frameChar * frameWidthChar, 0, frameWidthChar, frameHeightChar};
            SDL_Rect destChar = {(WINDOW_WIDTH - frameWidthChar)/2, (WINDOW_HEIGHT - frameHeightChar)/2 + 30, frameWidthChar, frameHeightChar};
            SDL_RenderCopy(game.renderer, game.character, &srcChar, &destChar); 

            char timeText[32];
            snprintf(timeText, sizeof(timeText), "%02d:%02d", remaining / 60, remaining % 60);
            
            SDL_Color timerColor = isResting ? (SDL_Color){100, 255, 100, 255} : (SDL_Color){255, 255, 255, 255};
            renderText(game.renderer, game.fontTimer, timeText, 40, WINDOW_HEIGHT - 25, timerColor);
        }

        SDL_RenderPresent(game.renderer);
    }

    cleanup(&game);
    return 0;
}