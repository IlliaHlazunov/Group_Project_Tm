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

// координати вікна таймера
#define WINDOW_POS_X 150
#define WINDOW_POS_Y 0

// змінні для вікна і рендера
SDL_Window *window;
SDL_Renderer *renderer;

// картинки
SDL_Texture *texMap;
SDL_Texture *texCat;
SDL_Texture *texWindow;
SDL_Texture *bgLayers[4];

// шрифти
TTF_Font *fontSmall;
TTF_Font *fontBig;
TTF_Font *fontTitle;

// розміри картинок
int catW = 0, catH = 0;
int winW = 0, winH = 0;

// зони на карті
typedef struct {
    int x, y;
    int workTime;
    int restTime;
    SDL_Rect hitBox;
} Zone;

Zone zones[3];

// стан гри
int scene = 0; // 0 меню, 1 карта, 2 таймер
bool isRunning = true;
Uint32 timerStart = 0;
int totalSeconds = 0;
bool isResting = false;
int currentZone = 0;

// чи відкрито вікно
bool isWindowOpen = false; 

// анімація
int bgFrame = 0;
int bgSheet = 0;
int charFrame = 0;
int winFrame = 0;

// таймери для анімації
Uint32 lastBgTime = 0;
Uint32 lastCharTime = 0;
Uint32 lastWinTime = 0;

// функція щоб малювати текст
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

// тут ініціалізація всього
int init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 0;
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) return 0;
    if (TTF_Init() < 0) return 0;

    window = SDL_CreateWindow("Muskrat", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIN_W, WIN_H, SDL_WINDOW_SHOWN);
    if (!window) return 0;
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) return 0;

    // грузимо шрифти
    fontSmall = TTF_OpenFont("assets/OpenSans-Regular.ttf", 12);
    if (!fontSmall) fontSmall = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 12);
    
    fontBig = TTF_OpenFont("assets/OpenSans-Regular.ttf", 20);
    if (!fontBig) fontBig = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 20);
    
    fontTitle = TTF_OpenFont("assets/OpenSans-Regular.ttf", 32);
    if (!fontTitle) fontTitle = TTF_OpenFont("C:\\Windows\\Fonts\\arialbd.ttf", 32);

    // грузимо текстури
    texMap = IMG_LoadTexture(renderer, "assets/karta.png");
    texCat = IMG_LoadTexture(renderer, "assets/characterAn.png");
    
    texWindow = IMG_LoadTexture(renderer, "window.png");
    if (!texWindow) {
        texWindow = IMG_LoadTexture(renderer, "assets/window.png");
    }

    if (texWindow) {
        SDL_QueryTexture(texWindow, NULL, NULL, &winW, &winH);
        printf("WINDOW LOADED! Size: %dx%d\n", winW, winH);
    } else {
        printf("ERROR: Window texture missing! Put 'window.png' next to exe or in assets.\n");
    }

    if(texCat) SDL_QueryTexture(texCat, NULL, NULL, &catW, &catH);

    for (int i = 0; i < 4; i++) {
        char path[64];
        sprintf(path, "assets/backgroundAn_%d.png", i + 1);
        bgLayers[i] = IMG_LoadTexture(renderer, path);
    }

    return 1;
}

// чистимо память
void cleanup() {
    for (int i = 0; i < 4; i++) if(bgLayers[i]) SDL_DestroyTexture(bgLayers[i]);
    if(texCat) SDL_DestroyTexture(texCat);
    if(texMap) SDL_DestroyTexture(texMap);
    if(texWindow) SDL_DestroyTexture(texWindow);
    
    if(fontSmall) TTF_CloseFont(fontSmall);
    if(fontBig) TTF_CloseFont(fontBig);
    if(fontTitle) TTF_CloseFont(fontTitle);
    
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

    zones[0] = (Zone){245, 50, 0, 0, {185, 30, 70, 45}};
    zones[1] = (Zone){58, 230, 0, 0, {35, 200, 70, 45}};
    zones[2] = (Zone){237, 240, 0, 0, {195, 210, 70, 45}};

    // рандомний час
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

    SDL_Event e;
    Uint32 frameStart;
    int frameTime;

    // головний цикл
    while (isRunning) {
        frameStart = SDL_GetTicks();

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) isRunning = false;

            if (scene == 0) { 
                if (e.type == SDL_KEYDOWN || e.type == SDL_MOUSEBUTTONDOWN) {
                    scene = 1;
                }
            }
            else if (scene == 1) { 
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
            else if (scene == 2) { 
                // клік по вікну
                if (e.type == SDL_MOUSEBUTTONDOWN) {
                    SDL_Point click = {e.button.x, e.button.y};
                    
                    int frameH = winH / 6;
                    
                    // зменшення зони кліку
                    int hitboxShrink = 15; 
                    
                    SDL_Rect winHitbox = {
                        WINDOW_POS_X + hitboxShrink, 
                        WINDOW_POS_Y + hitboxShrink, 
                        winW - (hitboxShrink * 2), 
                        frameH - (hitboxShrink * 2)
                    };
                    
                    if (SDL_PointInRect(&click, &winHitbox)) {
                        isWindowOpen = !isWindowOpen;
                        printf("Window toggled. Open: %d\n", isWindowOpen);
                    }
                }

                if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_c) {
                    const Uint8 *keys = SDL_GetKeyboardState(NULL);
                    if (keys[SDL_SCANCODE_SPACE]) {
                        timerStart -= 40000;
                    }
                }
            }
        }

        SDL_RenderClear(renderer);

        // малюю меню
        if (scene == 0) {
            SDL_SetRenderDrawColor(renderer, 135, 206, 250, 255);
            SDL_RenderFillRect(renderer, NULL);

            renderText(fontTitle, "MUSKRAT", WIN_W/2 + 2, 102, (SDL_Color){0, 50, 100, 100});
            renderText(fontTitle, "MUSKRAT", WIN_W/2, 100, (SDL_Color){255, 255, 255, 255});
            
            if ((SDL_GetTicks() / 500) % 2) {
                 renderText(fontSmall, "- PRESS START -", WIN_W/2, WIN_H - 50, (SDL_Color){50, 80, 120, 255});
            }
        }
        // малюю карту
        else if (scene == 1) {
            SDL_RenderCopy(renderer, texMap, NULL, NULL);

            for (int i = 0; i < 3; i++) {
                char buffer[32];
                sprintf(buffer, "%d / %d", zones[i].workTime, zones[i].restTime);
                renderText(fontSmall, buffer, zones[i].x, zones[i].y, (SDL_Color){255, 255, 255, 255});
            }
        }
        // малюю таймер і кота
        else if (scene == 2) {
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

            // анімація вікна
            if (isWindowOpen) {
                winFrame = 5; 
            } else {
                winFrame = 0; 
            }

            int bw, bh;
            if(bgLayers[bgSheet]) {
                SDL_QueryTexture(bgLayers[bgSheet], NULL, NULL, &bw, &bh);
                SDL_Rect srcBg = {0, bgFrame * (bh / 6), bw, bh / 6};
                SDL_RenderCopy(renderer, bgLayers[bgSheet], &srcBg, NULL);
            }

            // малюю саме вікно
            if (texWindow) {
                int frameH = winH / 6;
                if (frameH > 0) {
                    SDL_Rect srcWin = {0, winFrame * frameH, winW, frameH};
                    SDL_Rect dstWin = {WINDOW_POS_X, WINDOW_POS_Y, winW, frameH}; 
                    SDL_RenderCopy(renderer, texWindow, &srcWin, &dstWin);
                }
            }

            if(texCat) {
                int cw = catW / 4;
                SDL_Rect srcCat = {charFrame * cw, 0, cw, catH};
                SDL_Rect dstCat = {(WIN_W - cw)/2, (WIN_H - catH)/2 + 30, cw, catH};
                SDL_RenderCopy(renderer, texCat, &srcCat, &dstCat);
            }

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