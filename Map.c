#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define WIN_W 300
#define WIN_H 300
#define BG_DELAY 100
#define CHAR_DELAY 150
#define SHEETS 4
#define FRAMES 6
#define CHAR_FRAMES 4

typedef struct {
    int x, y;
    int workTime, restTime;
    SDL_Rect hitBox;
} Flag;

typedef struct {
    SDL_Window *win;
    SDL_Renderer *ren;
    SDL_Texture *map;
    SDL_Texture *bg[SHEETS];
    SDL_Texture *cat;
    TTF_Font *fontSmall;
    TTF_Font *fontBig;
} App;

Flag flags[3];

// --- ДОПОМІЖНІ ФУНКЦІЇ ---

// Генерація: 4, 6, 8, 10, 12, 14 (ХВИЛИНИ)
void randomizeFlags() {
    for (int i = 0; i < 3; i++) flags[i].workTime = -1; 

    for (int i = 0; i < 3; i++) {
        int val;
        bool isDup;
        do {
            // Формула: 4 + (0..5)*2 -> 4, 6, 8, 10, 12, 14
            val = 4 + (rand() % 6) * 2; 
            isDup = false;
            for (int j = 0; j < i; j++) 
                if (flags[j].workTime == val) isDup = true;
        } while (isDup);

        flags[i].workTime = val;       // Хвилини роботи
        flags[i].restTime = val / 2;   // Хвилини відпочинку
    }
}

// Малювання тексту
void drawText(SDL_Renderer *ren, TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
    if (!font) return;
    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, text, color);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
        SDL_Rect r = {x - surf->w / 2, y - surf->h / 2, surf->w, surf->h};
        SDL_RenderCopy(ren, tex, NULL, &r);
        SDL_FreeSurface(surf);
        SDL_DestroyTexture(tex);
    }
}

// Ініціалізація
bool initApp(App *app) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || !(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) || TTF_Init() < 0) return false;

    app->win = SDL_CreateWindow("Muskrat", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIN_W, WIN_H, SDL_WINDOW_SHOWN);
    app->ren = SDL_CreateRenderer(app->win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
    // --- ШРИФТИ ---
    // fontSmall (для карти) -> ТЕПЕР 12
    app->fontSmall = TTF_OpenFont("assets/OpenSans-Regular.ttf", 12);
    if (!app->fontSmall) app->fontSmall = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 12);
    
    // fontBig (для таймера) -> 20
    app->fontBig = TTF_OpenFont("assets/OpenSans-Regular.ttf", 20);
    if (!app->fontBig) app->fontBig = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 20);

    // Картинки
    app->map = IMG_LoadTexture(app->ren, "assets/karta.png");
    app->cat = IMG_LoadTexture(app->ren, "assets/characterAn.png");
    for (int i = 0; i < SHEETS; i++) {
        char path[64];
        sprintf(path, "assets/backgroundAn_%d.png", i + 1);
        app->bg[i] = IMG_LoadTexture(app->ren, path);
    }

    return (app->ren && app->map && app->cat && app->bg[0] && app->fontSmall);
}

void closeApp(App *app) {
    for (int i = 0; i < SHEETS; i++) SDL_DestroyTexture(app->bg[i]);
    SDL_DestroyTexture(app->cat);
    SDL_DestroyTexture(app->map);
    TTF_CloseFont(app->fontSmall);
    TTF_CloseFont(app->fontBig);
    SDL_DestroyRenderer(app->ren);
    SDL_DestroyWindow(app->win);
    TTF_Quit(); IMG_Quit(); SDL_Quit();
}

// --- ГОЛОВНА ФУНКЦІЯ ---
int main() {
    srand((unsigned int)time(NULL));
    App app = {0};
    if (!initApp(&app)) return 1;

    // Координати прапорців 
    flags[0] = (Flag){245, 50, 0, 0, {185, 30, 70, 45}};
    flags[1] = (Flag){58, 230, 0, 0, {35, 200, 70, 45}};
    flags[2] = (Flag){237, 240, 0, 0, {195, 210, 70, 45}};
    randomizeFlags();

    // Змінні стану гри
    int scene = 0; // 0 = MAP, 1 = CAT
    bool running = true;
    SDL_Event e;

    // Змінні таймера і кота
    Uint32 timerStart = 0, lastBG = 0, lastChar = 0;
    int totalTime = 0, activeFlag = -1;
    bool isRest = false;
    
    // Анімація (розміри)
    int sheetIdx = 0, frameBG = 0, frameChar = 0;
    int bgW, bgH, catW, catH;
    SDL_QueryTexture(app.cat, NULL, NULL, &catW, &catH);
    int frameW = catW / CHAR_FRAMES;

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;

            // Клік на карті
            if (scene == 0 && e.type == SDL_MOUSEBUTTONDOWN) {
                SDL_Point m = {e.button.x, e.button.y};
                for (int i = 0; i < 3; i++) {
                    if (SDL_PointInRect(&m, &flags[i].hitBox)) {
                        scene = 1; // Перехід до кота
                        activeFlag = i;
                        isRest = false;
                        // ВАЖЛИВО: Множимо хвилини на 60
                        totalTime = flags[i].workTime * 60; 
                        timerStart = SDL_GetTicks();
                    }
                }
            }

            // ЧІТ-КОД: "Space" + "C" = Мінус 40 секунд
            if (scene == 1 && e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_c) {
                    const Uint8 *keys = SDL_GetKeyboardState(NULL);
                    if (keys[SDL_SCANCODE_SPACE]) {
                        timerStart -= 40000; 
                    }
                }
            }
        }

        SDL_RenderClear(app.ren);

        if (scene == 0) { // === КАРТА ===
            SDL_RenderCopy(app.ren, app.map, NULL, NULL);
            SDL_Color white = {255, 255, 255, 255};
            for (int i = 0; i < 3; i++) {
                char buf[16];
                // Виводимо хвилини (наприклад, "14 / 7")
                snprintf(buf, sizeof(buf), "%d / %d", flags[i].workTime, flags[i].restTime);
                // Використовуємо fontSmall (розмір 12)
                drawText(app.ren, app.fontSmall, buf, flags[i].x, flags[i].y, white);
            }
        } 
        else {
            FILE *file = fopen("TimeToGo.txt", "w");
            if (file != NULL) {
                fprintf(file, "w%dr%d", flags[activeFlag].workTime, flags[activeFlag].restTime);
                fclose(file);
            }
            system("start Main_Screen.exe");
            return 0;
        }

        SDL_RenderPresent(app.ren);
    }

    closeApp(&app);
    return 0;
}