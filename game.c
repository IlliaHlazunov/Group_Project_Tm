#define SDL_MAIN_HANDLED
#include <game.h>
  /*
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
   */

#define WINDOW_TITLE "Muskrat (MAIN SCREEN)"
#define SCREEN_WIDTH 300
#define SCREEN_HEIGHT 300
#define IMAGE_FLAGS IMG_INIT_PNG

#define FRAME_DELAY_BG 100
#define FRAMES_PER_SHEET 6
#define TOTAL_SHEETS 4
#define FRAME_DELAY_CHAR 150

int FoodEnergy = 4000;
int EnergyLeft = 6;

struct Game {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *backgroundSheets[TOTAL_SHEETS];
    SDL_Texture *character;
    SDL_Texture *fishBox;
    SDL_Texture *clock;
    TTF_Font *fontTimer;
};

struct Game game_global = {0};

bool FoodGrabbed = false;
int FishLeft = 4;
bool isTriggerOpen = false;

typedef struct {
    SDL_Rect rect;
    SDL_Color colors[4];
    int color_index;
    bool cursorOnButton;
    void (*action)(void);

    SDL_Texture* texture;
    int frame;
    int total_frames;
    Uint32 last_frame_time;
    int frame_delay;
} Button;

typedef struct {
    SDL_Rect rect;
    SDL_Color colors[2];
    int color_index;
    bool cursorOnButton;
    void (*action)(void);
} Button_Eat;

typedef struct {
    SDL_Rect rect;
    SDL_Color colors[1];
    int color_index;
    bool cursorOnButton;
} Eat_trigger;

typedef struct {
    SDL_Rect rect;
    SDL_Color colors[1];
    int color_index;
    SDL_Texture* texture;
} Da_Fish;

typedef struct {
    SDL_Rect rect;
    SDL_Color colors[1];
    int color_index;
    SDL_Texture* texture;
} Da_Food;

typedef struct {
    SDL_Rect rect;
    SDL_Color colors[1];
    int color_index;
} Hunger_Element;

void button_click(void);
void pickup_fish(void);
void feed_fish(void);
void game_cleanup(struct Game *game, int exit_status);
void updt_food_indicators(SDL_Renderer* renderer, Da_Food* f1, Da_Food* f2, Da_Food* f3, Da_Food* f4, Da_Food* f5, Da_Food* f6);


Button create_button(int x, int y, int w, int h, void (*action_func)(void), SDL_Texture* tex, int total_frames, int frame_delay) {
    Button btn;
    btn.rect.x = x; 
    btn.rect.y = y; 
    btn.rect.w = w; 
    btn.rect.h = h;
    btn.colors[0] = (SDL_Color){255,0,0,255};
    btn.colors[1] = (SDL_Color){255,165,0,255};
    btn.colors[2] = (SDL_Color){0,255,0,255};
    btn.colors[3] = (SDL_Color){255,255,0,255};
    btn.color_index = 0;
    btn.cursorOnButton = false;
    btn.action = action_func;

    btn.texture = tex;
    btn.frame = 0;
    btn.total_frames = total_frames;
    btn.last_frame_time = SDL_GetTicks();
    btn.frame_delay = frame_delay;
    return btn;
}

Da_Food create_FOOD(int x, int y, int w, int h, SDL_Texture* tex) {
    Da_Food food;
    food.rect.x = x;
    food.rect.y = y;
    food.rect.w = w;
    food.rect.h = h;
    food.colors[0] = (SDL_Color){255, 255, 255, 255};
    food.color_index = 0;
    food.texture = tex;
    return food;
}

Button_Eat create_button_eat(int x, int y, int w, int h, void (*action_func)(void)) {
    Button_Eat btn;
    btn.rect.x = x; 
    btn.rect.y = y; 
    btn.rect.w = w; 
    btn.rect.h = h;
    btn.colors[0] = (SDL_Color){3,57, 108, 0};
    btn.colors[1] = (SDL_Color){0,91, 150, 0};
    btn.color_index = 0;
    btn.cursorOnButton = false;
    btn.action = action_func;
    return btn;
}

Eat_trigger create_eat_trigger(int x, int y, int w, int h) {
    Eat_trigger btn;
    btn.rect.x = x; 
    btn.rect.y = y; 
    btn.rect.w = w; 
    btn.rect.h = h;
    btn.colors[0] = (SDL_Color){179,205,224, 0};
    btn.color_index = 0;
    btn.cursorOnButton = false;
    return btn;
}

Da_Fish create_FISH(int x, int y, int w, int h, SDL_Texture* fishTexture) {
    Da_Fish btn;
    btn.rect.x = x; 
    btn.rect.y = y; 
    btn.rect.w = w; 
    btn.rect.h = h;
    btn.colors[0] = (SDL_Color){255, 255, 255, 255};
    btn.color_index = 0;
    btn.texture = fishTexture;
    return btn;
}

Hunger_Element create_Element(int x, int y, int w, int h) {
    Hunger_Element btn;
    btn.rect.x = x; 
    btn.rect.y = y; 
    btn.rect.w = w; 
    btn.rect.h = h;
    btn.colors[0] = (SDL_Color){155,206,120,255};
    btn.color_index = 0;
    return btn;
}

void renderTextCentered(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
    if (!font) { return; }
    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, text, color);
    if (!surf) { return; }
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_Rect rect = { x - surf->w/2, y - surf->h/2, surf->w, surf->h };
    SDL_RenderCopy(renderer, tex, NULL, &rect);
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);
}

void update_button_animation(Button* btn) {
    if (!btn || !btn->texture) { return; }
    Uint32 now = SDL_GetTicks();
    if (now - btn->last_frame_time >= (Uint32)btn->frame_delay) {
        if (btn->cursorOnButton) {
            if (btn->frame < btn->total_frames - 1) { btn->frame++; }
        } else {
            if (btn->frame > 0) { btn->frame--; }
        }
        btn->last_frame_time = now;
    }
}

void updt_FOOD(SDL_Renderer* renderer, Da_Food* food) {
    if (food && food->texture) {
        SDL_RenderCopy(renderer, food->texture, NULL, &food->rect);
    }
}

void updt_button(SDL_Renderer* renderer, Button* btn) {
    if (btn->texture) {
        int texW, texH;
        SDL_QueryTexture(btn->texture, NULL, NULL, &texW, &texH);
        int frame_height = texH / btn->total_frames;
        SDL_Rect src = {0, btn->frame * frame_height, texW, frame_height};
        SDL_RenderCopy(renderer, btn->texture, &src, &btn->rect);
    } else {
        SDL_SetRenderDrawColor(renderer,
            btn->colors[btn->color_index].r,
            btn->colors[btn->color_index].g,
            btn->colors[btn->color_index].b,
            btn->colors[btn->color_index].a);
        SDL_RenderFillRect(renderer, &btn->rect);
    }
}


void updt_button_eat(SDL_Renderer* renderer, Button_Eat* btn) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer,
        btn->colors[btn->color_index].r,
        btn->colors[btn->color_index].g,
        btn->colors[btn->color_index].b,
        btn->colors[btn->color_index].a);
    SDL_RenderFillRect(renderer, &btn->rect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void updt_eat_trigger(SDL_Renderer* renderer, Eat_trigger* btn) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer,
        btn->colors[btn->color_index].r,
        btn->colors[btn->color_index].g,
        btn->colors[btn->color_index].b,
        btn->colors[btn->color_index].a);
    SDL_RenderFillRect(renderer, &btn->rect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void updt_FISH(SDL_Renderer* renderer, Da_Fish* btn) {
    if (FoodGrabbed) {
        SDL_RenderCopy(renderer, btn->texture, NULL, &btn->rect);
    }
}


void updt_fish_indicators(SDL_Renderer* renderer, Da_Fish* f1, Da_Fish* f2, Da_Fish* f3, Da_Fish* f4) {
    if (FishLeft >= 1) { SDL_RenderCopy(renderer, f1->texture, NULL, &f1->rect); }
    if (FishLeft >= 2) { SDL_RenderCopy(renderer, f2->texture, NULL, &f2->rect); }
    if (FishLeft >= 3) { SDL_RenderCopy(renderer, f3->texture, NULL, &f3->rect); }
    if (FishLeft >= 4) { SDL_RenderCopy(renderer, f4->texture, NULL, &f4->rect); }
}

void updt_food_indicators(SDL_Renderer* renderer, Da_Food* f1, Da_Food* f2, Da_Food* f3, Da_Food* f4, Da_Food* f5, Da_Food* f6) {
    if (EnergyLeft >= 1) { SDL_RenderCopy(renderer, f1->texture, NULL, &f1->rect); }
    if (EnergyLeft >= 2) { SDL_RenderCopy(renderer, f2->texture, NULL, &f2->rect); }
    if (EnergyLeft >= 3) { SDL_RenderCopy(renderer, f3->texture, NULL, &f3->rect); }
    if (EnergyLeft >= 4) { SDL_RenderCopy(renderer, f4->texture, NULL, &f4->rect); }
    if (EnergyLeft >= 5) { SDL_RenderCopy(renderer, f5->texture, NULL, &f5->rect); }
    if (EnergyLeft >= 6) { SDL_RenderCopy(renderer, f6->texture, NULL, &f6->rect); }
}


void handle_button_event(SDL_Event* event, Button* btn) {
    if (!btn) { return; }
    int mouseX = 0, mouseY = 0;
    if (event->type == SDL_MOUSEMOTION) {
        mouseX = event->motion.x;
        mouseY = event->motion.y;
    } else if (event->type == SDL_MOUSEBUTTONDOWN) {
        mouseX = event->button.x;
        mouseY = event->button.y;
    } else {
        return;
    }

    bool inside = (mouseX >= btn->rect.x && mouseX <= btn->rect.x + btn->rect.w && mouseY >= btn->rect.y && mouseY <= btn->rect.y + btn->rect.h);
    if (event->type == SDL_MOUSEMOTION) {
        btn->cursorOnButton = inside;
    } else if (event->type == SDL_MOUSEBUTTONDOWN && inside && btn->action) {
        btn->action();
    }
}

void move_FISH(Da_Fish* btn) {
    int mouseX = 0, mouseY = 0;
    if (FoodGrabbed) {
        SDL_GetMouseState(&mouseX, &mouseY);
        btn->rect.x = mouseX - (btn->rect.w / 2);
        btn->rect.y = mouseY - (btn->rect.h / 2);
    }
}

void handle_button_event_eat(SDL_Event* event, Button_Eat* btn, Eat_trigger* trigger) {
    int mouseX = 0, mouseY = 0;
    if (event->type == SDL_MOUSEMOTION) {
        mouseX = event->motion.x;
        mouseY = event->motion.y;
    } else if (event->type == SDL_MOUSEBUTTONDOWN) {
        mouseX = event->button.x;
        mouseY = event->button.y;
    } else if (event->type == SDL_MOUSEBUTTONUP) {
        mouseX = event->button.x;
        mouseY = event->button.y;
    } else {
        return;
    }

    bool insideButton = (mouseX >= btn->rect.x && mouseX <= btn->rect.x + btn->rect.w && mouseY >= btn->rect.y && mouseY <= btn->rect.y + btn->rect.h);
    bool insideTrigger = (mouseX >= trigger->rect.x && mouseX <= trigger->rect.x + trigger->rect.w && mouseY >= trigger->rect.y && mouseY <= trigger->rect.y + trigger->rect.h);

    if (event->type == SDL_MOUSEMOTION) {
        btn->cursorOnButton = insideButton;
        trigger->cursorOnButton = insideTrigger;

        if (FoodGrabbed) {
            if (insideTrigger && !isTriggerOpen) {
                printf("Open\n");
                isTriggerOpen = true;
            } else if (!insideTrigger && isTriggerOpen) {
                printf("Close\n");
                isTriggerOpen = false;
            }
        }

    } else if (event->type == SDL_MOUSEBUTTONDOWN) {
        if (insideButton && FishLeft > 0) {
            FoodGrabbed = true;
            FishLeft -= 1;
            if(btn->action) {
                btn->action();
            }
            if (insideTrigger) {
               printf("Open\n");
               isTriggerOpen = true;
            }
        }
    } else if (event->type == SDL_MOUSEBUTTONUP) {
        if (FoodGrabbed) {
            if (insideTrigger) {
                feed_fish();
            } else {
                printf("LostF\n");
                FishLeft += 1;
            }
            FoodGrabbed = false;
            isTriggerOpen = false;
        }
    }
}

void button_click(void) {
    printf("Fish (window)\n");
}

void pickup_fish(void) {
    printf("Pick the fish up\n");
}

void feed_fish(void) {
    printf("Feed da fish\n");
    EnergyLeft += 1;
    if (EnergyLeft >= 7) {
        printf("EatEatEat\n");
        game_cleanup(&game_global, 0);
    }
}

bool sdl_initialize(struct Game *game) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL init error: %s\n", SDL_GetError());
        return true;
    }
    if (!(IMG_Init(IMAGE_FLAGS) & IMAGE_FLAGS)) {
        fprintf(stderr, "SDL_image init error: %s\n", IMG_GetError());
        return true;
    }
    if (TTF_Init() < 0) {
        fprintf(stderr, "TTF init error: %s\n", TTF_GetError());
        return true;
    }

    game->window = SDL_CreateWindow(WINDOW_TITLE,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!game->window) {
        fprintf(stderr, "Window create error: %s\n", SDL_GetError());
        return true;
    }

    game->renderer = SDL_CreateRenderer(game->window, -1, SDL_RENDERER_ACCELERATED);
    if (!game->renderer) {
        fprintf(stderr, "Renderer create error: %s\n", SDL_GetError());
        return true;
    }

    game->fontTimer = TTF_OpenFont("assets/OpenSans-Regular.ttf", 20);
    if (!game->fontTimer) {
        game->fontTimer = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 20);
    }

    return false;
}

bool load_media_combined(struct Game *game) {
    char filename[64];
    for (int i = 0; i < TOTAL_SHEETS; i++) {
        sprintf(filename, "assets/backgroundAn_%d.png", i + 1);
        game->backgroundSheets[i] = IMG_LoadTexture(game->renderer, filename);
    }

    game->character = IMG_LoadTexture(game->renderer, "assets/characterAn.png");
    game->fishBox = IMG_LoadTexture(game->renderer, "assets/FishBox.png");
    game->clock = IMG_LoadTexture(game->renderer, "assets/Clock.png");
    return false;
}

void game_cleanup(struct Game *game, int exit_status) {
    for (int i = 0; i < TOTAL_SHEETS; i++) {
        if (game->backgroundSheets[i]) { SDL_DestroyTexture(game->backgroundSheets[i]); }
    }
    if (game->character) { SDL_DestroyTexture(game->character); }
    if (game->fishBox) { SDL_DestroyTexture(game->fishBox); }
    if (game->clock) { SDL_DestroyTexture(game->clock); }
    if (game->fontTimer) { TTF_CloseFont(game->fontTimer); }
    if (game->renderer) { SDL_DestroyRenderer(game->renderer); }
    if (game->window) { SDL_DestroyWindow(game->window); }
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
    exit(exit_status);
}

int main(void) {
    if (sdl_initialize(&game_global)) {game_cleanup(&game_global, EXIT_FAILURE);}
    if (load_media_combined(&game_global)) {game_cleanup(&game_global, EXIT_FAILURE);}

    SDL_Texture* winTex = IMG_LoadTexture(game_global.renderer, "assets/window.png");
    Button top_right_button = create_button(SCREEN_WIDTH - 150, 0, 145, 145, button_click, winTex, 6, 100);

    Button_Eat box_eat = create_button_eat(SCREEN_WIDTH - 100, SCREEN_HEIGHT - 80, 100, 90, pickup_fish);
    Eat_trigger Trigger_eat = create_eat_trigger((SCREEN_WIDTH -100)/2, (SCREEN_HEIGHT-80)/2, 100, 105);

    SDL_Texture* fishTexture = IMG_LoadTexture(game_global.renderer, "assets/Fish.png");
    Da_Fish FISH = create_FISH(0, 0, 60, 60, fishTexture); 

    Da_Fish Fish1 = create_FISH(SCREEN_WIDTH - 120, SCREEN_HEIGHT - 73, 71, 71, fishTexture);Da_Fish Fish2 = create_FISH(SCREEN_WIDTH - 95,  SCREEN_HEIGHT - 78, 71, 71, fishTexture);Da_Fish Fish3 = create_FISH(SCREEN_WIDTH - 70,  SCREEN_HEIGHT - 76, 71, 71, fishTexture);Da_Fish Fish4 = create_FISH(SCREEN_WIDTH - 55,  SCREEN_HEIGHT - 74, 71, 71, fishTexture);

    SDL_Texture* FoodTexture = IMG_LoadTexture(game_global.renderer, "assets/FoodElement.png");
    Da_Food Food1 = create_FOOD(0, 10, 35, 35, FoodTexture);Da_Food Food2 = create_FOOD(15, 10, 35, 35, FoodTexture);Da_Food Food3 = create_FOOD(30, 10, 35, 35, FoodTexture);Da_Food Food4 = create_FOOD(45, 10, 35, 35, FoodTexture);Da_Food Food5 = create_FOOD(60, 10, 35, 35, FoodTexture);Da_Food Food6 = create_FOOD(75, 10, 35, 35, FoodTexture);

    bool running = true;
    SDL_Event event;

    int currentSheet = 0;
    int frameBG = 0;
    Uint32 lastFrameTimeBG = SDL_GetTicks();

    int texW = 0;
    int texH = 0;
    SDL_QueryTexture(game_global.backgroundSheets[0], NULL, NULL, &texW, &texH);
    int frameHeightBG = texH / FRAMES_PER_SHEET;

    int frameChar = 0;
    Uint32 lastFrameTimeChar = SDL_GetTicks();
    int total_frames_char = 4;
    int char_width = 0;
    int char_height = 0;
    SDL_QueryTexture(game_global.character, NULL, NULL, &char_width, &char_height);
    int frame_width_char = char_width / total_frames_char;
    int frame_height_char = char_height;

    int fishBoxW = 0;
    int fishBoxH = 0;
    SDL_QueryTexture(game_global.fishBox, NULL, NULL, &fishBoxW, &fishBoxH);
    
    int clockW = 0;
    int clockH = 0;
    SDL_QueryTexture(game_global.clock, NULL, NULL, &clockW, &clockH);

    SDL_Rect destBG = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};

    Uint32 timerStart = SDL_GetTicks();
    int selectedTotalTime = 20;
    bool isResting = false;
    bool timerFinished = false;
    
    Uint32 lastHungerTickTime = SDL_GetTicks();

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) { 
                running = false; 
                break; 
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) { 
                running = false; 
                break; 
            }

            if (!timerFinished) {
                handle_button_event(&event, &top_right_button);
                handle_button_event_eat(&event, &box_eat, &Trigger_eat);
            }
        }

        int elapsed = (SDL_GetTicks() - timerStart) / 1000;
        int remaining = selectedTotalTime - elapsed;
        if (remaining <= 0) {
            remaining = 0;
            timerFinished = true;
        }

        Uint32 now = SDL_GetTicks();

        if (!timerFinished) {
            
            if (now - lastHungerTickTime > (Uint32)FoodEnergy) {
                if (EnergyLeft > 0) {
                    EnergyLeft--;
                    printf("Energy left: %d\n", EnergyLeft); 
                }
                lastHungerTickTime = now;

                if (EnergyLeft <= 0) {
                    printf("No energy.\n");
                    running = false;
                }
            }
            
            if (now - lastFrameTimeBG > FRAME_DELAY_BG) {
                frameBG++;
                if (frameBG >= FRAMES_PER_SHEET) {
                    frameBG = 0;
                    currentSheet++;
                    if (currentSheet >= TOTAL_SHEETS) {
                        currentSheet = 0;
                    }
                    SDL_QueryTexture(game_global.backgroundSheets[currentSheet], NULL, NULL, &texW, &texH);
                    frameHeightBG = texH / FRAMES_PER_SHEET;
                }
                lastFrameTimeBG = now;
            }

            if (now > lastFrameTimeChar + FRAME_DELAY_CHAR) {
                frameChar = (frameChar + 1) % total_frames_char;
                lastFrameTimeChar = now;
            }

            update_button_animation(&top_right_button);
            move_FISH(&FISH);
        }

        SDL_RenderClear(game_global.renderer);

        SDL_Rect srcBG = {0, frameBG * frameHeightBG, texW, frameHeightBG};
        SDL_RenderCopy(game_global.renderer, game_global.backgroundSheets[currentSheet], &srcBG, &destBG);

        SDL_Rect srcChar = {frameChar * frame_width_char, 0, frame_width_char, frame_height_char};
        SDL_Rect destChar = {(SCREEN_WIDTH - frame_width_char) / 2,(SCREEN_HEIGHT - frame_height_char + 7) / 2 + 40,frame_width_char,frame_height_char};
        SDL_RenderCopy(game_global.renderer, game_global.character, &srcChar, &destChar);
        updt_food_indicators(game_global.renderer, &Food1, &Food2, &Food3, &Food4, &Food5, &Food6);
        updt_fish_indicators(game_global.renderer, &Fish1, &Fish2, &Fish3, &Fish4);
        SDL_Rect destFishBox = { SCREEN_WIDTH - 140, SCREEN_HEIGHT - 113, fishBoxW, fishBoxH };
        SDL_RenderCopy(game_global.renderer, game_global.fishBox, NULL, &destFishBox);

        
        updt_button(game_global.renderer, &top_right_button);
        updt_button_eat(game_global.renderer, &box_eat);
        updt_eat_trigger(game_global.renderer, &Trigger_eat);
        updt_FISH(game_global.renderer, &FISH);
        
        SDL_Rect destClock = {-15, 215, clockW, clockH };
        SDL_RenderCopy(game_global.renderer, game_global.clock, NULL, &destClock);

        SDL_Color timerColor = isResting ? (SDL_Color){100,255,100,255} : (SDL_Color){255,255,255,255};
        char timeText[32];
        snprintf(timeText, sizeof(timeText), "%02d:%02d", remaining / 60, remaining % 60);
        if (game_global.fontTimer) {
            renderTextCentered(game_global.renderer, game_global.fontTimer, timeText, 55, SCREEN_HEIGHT - 25, timerColor);
        }

        if (timerFinished) {
            if (game_global.fontTimer) { printf("RETURN TO THE MAP");}
            return 0;
        }

        SDL_RenderPresent(game_global.renderer);
        SDL_Delay(16);  
    }

    game_cleanup(&game_global, EXIT_SUCCESS);
    return 0;
}