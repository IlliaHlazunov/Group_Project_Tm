#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define WIN_W 300
#define WIN_H 300

// Глобальні змінні 
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

SDL_Texture *texMap = NULL;
SDL_Texture *texCat = NULL;
SDL_Texture *texFish = NULL;
SDL_Texture *bgLayers[4];

TTF_Font *fontSmall = NULL;
TTF_Font *fontBig = NULL;
TTF_Font *fontTitle = NULL;

int fishW = 0, fishH = 0;
int catW = 0, catH = 0;

// Структури
typedef struct {
    float x, y;
    float vx, vy;
    double angle, rotSpeed;
    float scale;
    Uint8 alpha; // Прозорість для туману
    SDL_Color color;
} Fish;

typedef struct {
    int x, y;
    int workTime;
    int restTime;
    SDL_Rect hitBox;
} Zone;

Fish fishes[30];
Zone zones[3];

// Змінні гри
int scene = 0; // 0 = MENU, 1 = MAP, 2 = TIMER
bool isRunning = true;
Uint32 timerStart = 0;
int totalSeconds = 0;
bool isResting = false;
int currentZone = 0;

// Анімація
int bgFrame = 0;
int bgSheet = 0;
int charFrame = 0;
Uint32 lastBgTime = 0;
Uint32 lastCharTime = 0;

// Функція для тексту
void renderText(TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
    if (!font) return;
    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, text, color);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect rect = {x - surf->w / 2, y - surf->h / 2, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &rect);
        SDL_FreeSurface(surf);
        SDL_DestroyTexture(tex);
    }
}

// --- ОСЬ ТУТ МАГІЯ ТУМАНУ ПОВЕРНУТА ---
void reset_fish_pos(int i, bool randomY) {
    int layer = rand() % 100;

    // Логіка шарів: Далеко -> Середньо -> Близько
    if (layer < 50) { 
        // ШАР 1: ДАЛЕКО (Сильний туман)
        // Маленькі, повільні і дуже прозорі (зливаються з фоном)
        fishes[i].scale = 0.15f + (rand() % 15) / 100.0f;
        fishes[i].alpha = 80 + rand() % 50; 
        fishes[i].vy = 0.2f + (rand() % 20) / 100.0f;
    } else if (layer < 85) {
        // ШАР 2: СЕРЕДНЄ
        fishes[i].scale = 0.35f + (rand() % 20) / 100.0f;
        fishes[i].alpha = 160 + rand() % 50;
        fishes[i].vy = 0.5f + (rand() % 30) / 100.0f;
    } else {
        // ШАР 3: БЛИЗЬКО 
        fishes[i].scale = 0.6f + (rand() % 20) / 100.0f;
        fishes[i].alpha = 255; // Непрозорі
        fishes[i].vy = 0.9f + (rand() % 50) / 100.0f;
    }
    
    // Кольори 
    int type = rand() % 3;
    if (type == 0) fishes[i].color = (SDL_Color){255, 255, 255, 255};      // Срібна
    else if (type == 1) fishes[i].color = (SDL_Color){100, 120, 180, 255}; // Темна
    else fishes[i].color = (SDL_Color){150, 200, 150, 255};                // Зелена

    int w = (int)(fishW * fishes[i].scale);
    int h = (int)(fishH * fishes[i].scale);
    
    fishes[i].x = rand() % (WIN_W + w) - w;
    
    if (randomY) fishes[i].y = rand() % WIN_H - WIN_H;
    else fishes[i].y = -h - (rand() % 100); // Трохи вище екрану
    
    fishes[i].vx = ((rand() % 20) - 10) / 40.0f;
    fishes[i].angle = rand() % 360;
    fishes[i].rotSpeed = ((rand() % 10) - 5) * 0.1f;
}

// Ініціалізація
int init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 0;
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) return 0;
    if (TTF_Init() < 0) return 0;

    window = SDL_CreateWindow("Muskrat", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIN_W, WIN_H, SDL_WINDOW_SHOWN);
    if (!window) return 0;
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) return 0;

    // Шрифти
    fontSmall = TTF_OpenFont("assets/OpenSans-Regular.ttf", 12);
    if (!fontSmall) fontSmall = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 12);
    
    fontBig = TTF_OpenFont("assets/OpenSans-Regular.ttf", 20);
    if (!fontBig) fontBig = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 20);
    
    fontTitle = TTF_OpenFont("assets/OpenSans-Regular.ttf", 32);
    if (!fontTitle) fontTitle = TTF_OpenFont("C:\\Windows\\Fonts\\arialbd.ttf", 32);

    // Текстури
    texMap = IMG_LoadTexture(renderer, "assets/karta.png");
    texCat = IMG_LoadTexture(renderer, "assets/characterAn.png");
    texFish = IMG_LoadTexture(renderer, "assets/fish.png");

    // Для прозорості риб
    SDL_SetTextureBlendMode(texFish, SDL_BLENDMODE_BLEND);

    SDL_QueryTexture(texFish, NULL, NULL, &fishW, &fishH);
    SDL_QueryTexture(texCat, NULL, NULL, &catW, &catH);

    for (int i = 0; i < 4; i++) {
        char path[64];
        sprintf(path, "assets/backgroundAn_%d.png", i + 1);
        bgLayers[i] = IMG_LoadTexture(renderer, path);
    }

    return 1;
}

void cleanup() {
    for (int i = 0; i < 4; i++) SDL_DestroyTexture(bgLayers[i]);
    SDL_DestroyTexture(texCat);
    SDL_DestroyTexture(texMap);
    SDL_DestroyTexture(texFish);
    TTF_CloseFont(fontSmall);
    TTF_CloseFont(fontBig);
    TTF_CloseFont(fontTitle);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    if (!init()) {
        printf("Error init\n");
        return 1;
    }

    // Зони
    zones[0] = (Zone){245, 50, 0, 0, {185, 30, 70, 45}};
    zones[1] = (Zone){58, 230, 0, 0, {35, 200, 70, 45}};
    zones[2] = (Zone){237, 240, 0, 0, {195, 210, 70, 45}};

    // Рандомний час
    for (int i = 0; i < 3; i++) {
        int val;
        int unique = 0;
        do {
            val = 4 + (rand() % 6) * 2;
            unique = 1;
            for (int j = 0; j < i; j++) {
                if (zones[j].workTime == val) unique = 0;
            }
        } while (!unique);
        zones[i].workTime = val;
        zones[i].restTime = val / 2;
    }

    // Спавн риб
    for (int i = 0; i < 30; i++) {
        reset_fish_pos(i, true);
    }

    SDL_Event e;
    Uint32 frameStart;
    int frameTime;

    while (isRunning) {
        frameStart = SDL_GetTicks();

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) isRunning = false;

            if (scene == 0) { // MENU
                if (e.type == SDL_KEYDOWN || e.type == SDL_MOUSEBUTTONDOWN) {
                    scene = 1;
                    // Скидаю прозорість, щоб на інших екранах (якщо треба) було ок
                    SDL_SetTextureColorMod(texFish, 255, 255, 255);
                    SDL_SetTextureAlphaMod(texFish, 255);
                }
            }
            else if (scene == 1) { // MAP
                if (e.type == SDL_MOUSEBUTTONDOWN) {
                    SDL_Point m = {e.button.x, e.button.y};
                    for (int i = 0; i < 3; i++) {
                        if (SDL_PointInRect(&m, &zones[i].hitBox)) {
                            scene = 2; 
                            currentZone = i;
                            isResting = false;
                            totalSeconds = zones[i].workTime * 60;
                            timerStart = SDL_GetTicks();
                        }
                    }
                }
            }
            else if (scene == 2) { // TIMER
                if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_c) {
                    const Uint8 *keys = SDL_GetKeyboardState(NULL);
                    if (keys[SDL_SCANCODE_SPACE]) {
                        timerStart -= 40000;
                    }
                }
            }
        }

        SDL_RenderClear(renderer);

        // --- МАЛЮВАННЯ 
        
        if (scene == 0) {
            // Фон меню (блакитний)
            SDL_SetRenderDrawColor(renderer, 135, 206, 250, 255);
            SDL_RenderFillRect(renderer, NULL);

            // риби рибок з ефектом туману
            for (int i = 0; i < 30; i++) {
                fishes[i].x += fishes[i].vx;
                fishes[i].y += fishes[i].vy;
                fishes[i].angle += fishes[i].rotSpeed;

                int fh = (int)(fishH * fishes[i].scale);
                // Якщо вилетіла за екран - респавн зверху
                if (fishes[i].y > WIN_H + fh) reset_fish_pos(i, false);

                SDL_Rect r = {(int)fishes[i].x, (int)fishes[i].y, (int)(fishW * fishes[i].scale), fh};
                
                // Застосовуємо колір і альфу (прозорість)
                SDL_SetTextureColorMod(texFish, fishes[i].color.r, fishes[i].color.g, fishes[i].color.b);
                SDL_SetTextureAlphaMod(texFish, fishes[i].alpha);
                
                SDL_RenderCopyEx(renderer, texFish, NULL, &r, fishes[i].angle, NULL, SDL_FLIP_NONE);
            }

            // Текст заголовка
            renderText(fontTitle, "MUSKRAT", WIN_W/2 + 2, 102, (SDL_Color){0, 50, 100, 100});
            renderText(fontTitle, "MUSKRAT", WIN_W/2, 100, (SDL_Color){255, 255, 255, 255});
            
            if ((SDL_GetTicks() / 500) % 2) {
                 renderText(fontSmall, "- PRESS START -", WIN_W/2, WIN_H - 50, (SDL_Color){50, 80, 120, 255});
            }
        }
        else if (scene == 1) {
            // Мапа
            SDL_SetTextureColorMod(texFish, 255, 255, 255);
            SDL_SetTextureAlphaMod(texFish, 255);
            SDL_RenderCopy(renderer, texMap, NULL, NULL);

            for (int i = 0; i < 3; i++) {
                char buffer[32];
                sprintf(buffer, "%d / %d", zones[i].workTime, zones[i].restTime);
                renderText(fontSmall, buffer, zones[i].x, zones[i].y, (SDL_Color){255, 255, 255, 255});
            }
        }
        else if (scene == 2) {
            // Таймер
            Uint32 now = SDL_GetTicks();
            int elapsed = (now - timerStart) / 1000;
            int left = totalSeconds - elapsed;

            if (left <= 0) {
                if (!isResting) {
                    isResting = true;
                    totalSeconds = zones[currentZone].restTime * 60;
                    timerStart = now;
                } else {
                    scene = 1; 
                    for(int i=0; i<3; i++) {
                         zones[i].workTime = 4 + (rand() % 6) * 2;
                         zones[i].restTime = zones[i].workTime / 2;
                    }
                }
            }

            // Анімація
            if (now - lastBgTime > 100) {
                bgFrame++;
                if (bgFrame >= 6) {
                    bgFrame = 0;
                    bgSheet++;
                    if (bgSheet >= 4) bgSheet = 0;
                }
                lastBgTime = now;
            }
            if (now - lastCharTime > 150) {
                charFrame = (charFrame + 1) % 4;
                lastCharTime = now;
            }

            int bw, bh;
            SDL_QueryTexture(bgLayers[bgSheet], NULL, NULL, &bw, &bh);
            SDL_Rect srcBg = {0, bgFrame * (bh / 6), bw, bh / 6};
            SDL_RenderCopy(renderer, bgLayers[bgSheet], &srcBg, NULL);

            int cw = catW / 4;
            SDL_Rect srcCat = {charFrame * cw, 0, cw, catH};
            SDL_Rect dstCat = {(WIN_W - cw)/2, (WIN_H - catH)/2 + 30, cw, catH};
            SDL_RenderCopy(renderer, texCat, &srcCat, &dstCat);

            char timeStr[16];
            if (left < 0) left = 0;
            sprintf(timeStr, "%02d:%02d", left / 60, left % 60);
            
            SDL_Color c = {255, 255, 255, 255};
            if (isResting) c = (SDL_Color){100, 255, 100, 255};
            
            renderText(fontBig, timeStr, 40, WIN_H - 25, c);
        }

        SDL_RenderPresent(renderer);
        
        frameTime = SDL_GetTicks() - frameStart;
        if (1000 / 60 > frameTime) {
            SDL_Delay(1000 / 60 - frameTime);
        }
    }

    cleanup();
    return 0;
}