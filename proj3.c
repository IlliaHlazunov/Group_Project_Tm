#define SDL_MAIN_HANDLED

#include <SDL.h>
#include <SDL_image.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define WINDOW_TITLE "03 Ver. with window color button"
#define SCREEN_WIDTH 350
#define SCREEN_HEIGHT 350
#define IMAGE_FLAGS IMG_INIT_PNG

struct Game {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *background;
    SDL_Texture *character;
};

typedef struct {
    SDL_Rect rect;
    SDL_Color colors[4];   
    int color_index;     
    bool cursorOnButton;        
    void (*action)(void);
} Button;

void button_click(void) {
    printf("Fish (window)\n");
}

Button create_button(int x, int y, int w, int h, void (*action_func)(void)) {
    Button btn;
    btn.rect.x = x;
    btn.rect.y = y;
    btn.rect.w = w;
    btn.rect.h = h;
    btn.colors[0] = (SDL_Color){255, 0, 0, 255};  
    btn.colors[1] = (SDL_Color){255, 165, 0, 255};
    btn.colors[2] = (SDL_Color){0, 255, 0, 255};    
    btn.colors[3] = (SDL_Color){255, 255, 0, 255}; 
    btn.color_index = 0;
    btn.cursorOnButton = false;
    btn.action = action_func;
    return btn;
}

void updt_button(SDL_Renderer* renderer, Button* btn) {
    SDL_SetRenderDrawColor(renderer,
        btn->colors[btn->color_index].r,
        btn->colors[btn->color_index].g,
        btn->colors[btn->color_index].b,
        btn->colors[btn->color_index].a);
    SDL_RenderFillRect(renderer, &btn->rect);
}

void handle_button_event(SDL_Event* event, Button* btn) {
    int mouseX, mouseY;
    if (event->type == SDL_MOUSEMOTION || event->type == SDL_MOUSEBUTTONDOWN) {
        mouseX = event->motion.x;
        mouseY = event->motion.y;
        bool inside = (mouseX >= btn->rect.x && mouseX <= btn->rect.x + btn->rect.w &&
                       mouseY >= btn->rect.y && mouseY <= btn->rect.y + btn->rect.h);

        if (event->type == SDL_MOUSEMOTION) {
            btn->cursorOnButton = inside;
        } else if (event->type == SDL_MOUSEBUTTONDOWN && inside && btn->action) {
            btn->action();
        }
    }
}

void update_button_color(Button* btn) {
    if (btn->cursorOnButton) {
        if (btn->color_index < 3) {
            btn->color_index++;
        }
    } else {
        if (btn->color_index > 0) { 
            btn->color_index--;
        }
    }
}

bool load_media(struct Game *game) {
    game->background = IMG_LoadTexture(game->renderer, "assets/background.png");
    if (!game->background) {
        fprintf(stderr, "Error loading background: %s\n", IMG_GetError());
        SDL_Delay(3000);
        return true;
    }

    game->character = IMG_LoadTexture(game->renderer, "assets/character.png");
    if (!game->character) {
        fprintf(stderr, "Error loading character: %s\n", IMG_GetError());
        SDL_Delay(3000);
        return true;
    }

    return false;
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

void game_cleanup(struct Game *game, int exit_status) {
    if (game->character) SDL_DestroyTexture(game->character);
    if (game->background) SDL_DestroyTexture(game->background);
    if (game->renderer) SDL_DestroyRenderer(game->renderer);
    if (game->window) SDL_DestroyWindow(game->window);
    IMG_Quit();
    SDL_Quit();
    exit(exit_status);
}

int main(void) {
    struct Game game = {0};

    if (sdl_initialize(&game)) {
        game_cleanup(&game, EXIT_FAILURE);
    }

    if (load_media(&game)) {
        game_cleanup(&game, EXIT_FAILURE);
    }

    Button top_right_button = create_button(SCREEN_WIDTH - 110, 10, 100, 90, button_click);

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                running = false;
            }

            handle_button_event(&event, &top_right_button);
        }

        update_button_color(&top_right_button);

        SDL_RenderClear(game.renderer);

        SDL_RenderCopy(game.renderer, game.background, NULL, NULL);

        int char_width, char_height;
        SDL_QueryTexture(game.character, NULL, NULL, &char_width, &char_height);
        SDL_Rect char_dest = {(SCREEN_WIDTH - char_width)/2, (SCREEN_HEIGHT - char_height)/2, char_width, char_height};
        SDL_RenderCopy(game.renderer, game.character, NULL, &char_dest);

        updt_button(game.renderer, &top_right_button);

        SDL_RenderPresent(game.renderer);
        SDL_Delay(100); 
    }

    game_cleanup(&game, EXIT_SUCCESS);
    return 0;
}
