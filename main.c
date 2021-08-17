#include <SDL2/SDL.h>
#include <SDL2_image/SDL_image.h>
#include <stdio.h>
#include <stdbool.h>

// MACROS
#define MAX(a, b) ((a > b) ? a : b)
#define MIN(a, b) ((a < b) ? a : b)
#define CLAMP(x, min, max) MIN(MAX(x, min), max)

// CONSTANTS

// Screen dimension constants
#define INIT_SCREEN_WIDTH 800
#define INIT_SCREEN_HEIGHT 600

// Game width inside window dimension
#define INIT_GAME_WIDTH 400

// Scaled width of player (256 x 256 image scaled)
#define PLAYER_W (int)( 256 * 0.2f )
#define PLAYER_H (int)( 256 * 0.2f )

// Stars.png height
#define STARS_HEIGHT 1024

// frames per second
#define FPS 60

// speeds
#define PLAYER_SPEED 5
#define BULLET_SPEED 5

// maxes
#define MAX_BULLETS 512
#define MAX_ENEMIES 256

// ENUMS
enum bullet_type {
    BULLET
};

// STRUCTS
struct vec2 {
    float x;
    float y;
};

struct bullet {
    enum bullet_type type;
    struct vec2 vel;
    struct vec2 pos;
};

// FUNCTION PROTOTYPES

void SDL_err_check_null(void* res);

void SDL_err_check_true(int res);

SDL_Texture* load_texture(SDL_Renderer* renderer, char* path);

// MAIN

int main() {

    // INITIALIZE RENDERING

    SDL_Window* window = NULL;
    SDL_Surface* screen_surface = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_Texture* stars_texture = NULL;
    SDL_Texture* player_ship_texture = NULL;
    SDL_Texture* enemy_ship_texture = NULL;
    SDL_Texture* beams_texture = NULL;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    }

    // Initialize SDL_Image
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        printf("SDL_image could not initialize! SDL_image Error: %s\n",
               IMG_GetError());
    }

    // Create window
    window = SDL_CreateWindow("Space Hell", SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, INIT_SCREEN_WIDTH,
                              INIT_SCREEN_HEIGHT,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    SDL_err_check_null(window);

    // Get window surface
    screen_surface = SDL_GetWindowSurface(window);
    SDL_err_check_null(screen_surface);

    // Get renderer
    renderer = SDL_GetRenderer(window);
    SDL_err_check_null(renderer);

    // Load textures
    stars_texture = load_texture(renderer, "../../sprites/stars.png");
    player_ship_texture = load_texture(renderer, "../../sprites/player-ship.png");
    enemy_ship_texture = load_texture(renderer, "../../sprites/enemy-ship.png");
    beams_texture = load_texture(renderer, "../../sprites/beams.png");

    // Define crops
    SDL_Rect bullet_yellow_big_crop = {225, 0, 70, 90};

    // INITIALIZE GAME STATE

    Uint32 prev_time = SDL_GetTicks(); // time of previous iteration in ms
    Uint32 delta_time; // time passed since previous iteration in ms

    int stars_top = 0; // top of stars

    struct vec2 player_vel = {0, 0};
    struct vec2 player_pos = {0, -INIT_SCREEN_HEIGHT / 4.0f};
    struct vec2 player_hitbox = {PLAYER_W / 2, PLAYER_H};

    int n_enemies = 0;
    struct vec2 enemy_pos[MAX_ENEMIES];

    n_enemies++;
    enemy_pos[0] = (struct vec2) { 0, 0 };

    int n_bullets = 0;
    struct bullet bullets[MAX_BULLETS];

    // MAIN LOOP

    while (true) {
        // PROCESS SDL EVENTS

        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                goto MAIN_LOOP_END;
            } else if (event.type == SDL_KEYDOWN) {
                SDL_Scancode key = event.key.keysym.scancode;
                if (key == SDL_SCANCODE_D) {
                    player_vel.x = PLAYER_SPEED;
                } else if (key == SDL_SCANCODE_A) {
                    player_vel.x = -PLAYER_SPEED;
                } else if (key == SDL_SCANCODE_W) {
                    player_vel.y = PLAYER_SPEED;
                } else if (key == SDL_SCANCODE_S) {
                    player_vel.y = -PLAYER_SPEED;
                } else if (key == SDL_SCANCODE_SPACE) {
                    bullets[n_bullets] = (struct bullet) {BULLET, {0, BULLET_SPEED},
                                                          {player_pos.x, player_pos.y + PLAYER_H / 2}};
                    n_bullets++;
                }
            } else if (event.type == SDL_KEYUP) {
                SDL_Scancode key = event.key.keysym.scancode;
                if (key == SDL_SCANCODE_D && player_vel.x > 0) {
                    player_vel.x = 0;
                } else if (key == SDL_SCANCODE_A && player_vel.x < 0) {
                    player_vel.x = 0;
                } else if (key == SDL_SCANCODE_W && player_vel.y > 0) {
                    player_vel.y = 0;
                } else if (key == SDL_SCANCODE_S && player_vel.y < 0) {
                    player_vel.y = 0;
                }
            }
        }

        // Get window size and scale
        int window_w, window_h;
        SDL_GetWindowSize(window, &window_w, &window_h);
        float scale_w = (float) window_w / INIT_SCREEN_WIDTH;
        float scale_h = (float) window_h / INIT_SCREEN_HEIGHT;

        // UPDATE GAME STATE

        // Compute delta time

        delta_time = SDL_GetTicks() - prev_time;
        prev_time += delta_time;

        // Update top of stars
        stars_top += 1;
        stars_top %= (int) (window_h - STARS_HEIGHT * scale_h);

        // Update player position
        player_pos.x = CLAMP(player_pos.x + player_vel.x, (player_hitbox.x - INIT_GAME_WIDTH) / 2,
                             (INIT_GAME_WIDTH - player_hitbox.x) / 2);
        player_pos.y = CLAMP(player_pos.y + player_vel.y, -(INIT_SCREEN_HEIGHT - player_hitbox.y) / 2,
                             (INIT_SCREEN_HEIGHT - player_hitbox.y) / 2);

        // Update bullets
        for (int i = 0; i < n_bullets; i++) {
            bullets[i].pos.x += bullets[i].vel.x;
            bullets[i].pos.y += bullets[i].vel.y;

            if (abs((int) bullets[i].pos.x) > INIT_GAME_WIDTH / 2 ||
                abs((int) bullets[i].pos.x) > INIT_SCREEN_HEIGHT / 2) {

                // Destroy bullet
                bullets[i] = bullets[n_bullets - 1];

                n_bullets--;
                i--;
            }
        }

        // RENDER TO SCREEN

        // Compute scaled values
        int game_area_w = (int) (INIT_GAME_WIDTH * scale_w);
        int player_w = (int) (PLAYER_W * scale_w);
        int player_h = (int) (PLAYER_H * scale_h);

        // Update rects according to scaled values
        SDL_Rect game_area_rect = {(window_w - game_area_w) / 2, 0, game_area_w, window_h};
        SDL_Rect stars_clip = {0, stars_top, game_area_w, window_h};
        SDL_Rect player_rect = {window_w / 2 - player_w / 2 + player_pos.x * scale_w,
                                window_h / 2 - player_h / 2 - player_pos.y * scale_h,
                                player_w, player_h};

        // Render background
        SDL_err_check_true(
                SDL_SetRenderDrawColor(renderer, 0x22, 0x22, 0x22, 0xFF));
        SDL_err_check_true(SDL_RenderClear(renderer));
        SDL_err_check_true(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF));
        SDL_err_check_true(SDL_RenderFillRect(renderer, &game_area_rect));

        // Render stars
        SDL_err_check_true(
                SDL_RenderCopy(renderer, stars_texture, &stars_clip, &game_area_rect));

        // Render ships
        SDL_err_check_true(
                SDL_RenderCopy(renderer, player_ship_texture, NULL, &player_rect));

        // Render bullets
        for (int i = 0; i < n_bullets; i++) {
            int bullet_w = 50 * scale_w;
            int bullet_h = 50 * scale_h;
            SDL_Rect bullet_rect = {window_w / 2 - bullet_w / 2 + bullets[i].pos.x * scale_w,
                                    window_h / 2 - bullet_h / 2 - bullets[i].pos.y * scale_h, bullet_w, bullet_h};
            SDL_err_check_true(SDL_RenderCopy(renderer, beams_texture, &bullet_yellow_big_crop, &bullet_rect));
        }

        // Render enemies
        for (int i = 0; i < n_enemies; i++) {
            SDL_Rect enemy_rect = { enemy_pos[i].x, enemy_pos[i].y };
        }

        // Update screen
        SDL_RenderPresent(renderer);

        // FPS
        SDL_Delay(1000 / FPS);
    }

    MAIN_LOOP_END:;

    // CLEANUP

    // Free loaded images
    SDL_DestroyTexture(stars_texture);
    SDL_DestroyTexture(player_ship_texture);

    // Destroy window
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    // Quit SDL subsystems
    IMG_Quit();
    SDL_Quit();

    return 0;
}

// FUNCTION IMPLEMENTATIONS

void SDL_err_check_null(void* res) {
    if (res == NULL) {
        printf("SDL call returned NULL for an error! SDL_Error: %s\n",
               SDL_GetError());
    }
}

void SDL_err_check_true(int res) {
    if (res < 0) {
        printf("SDL call returned true for an error! SDL_Error: %s\n",
               SDL_GetError());
    }
}

SDL_Texture* load_texture(SDL_Renderer* renderer, char* path) {

    // The final optimized texture
    SDL_Texture* new_texture = NULL;

    // Load image at specified path
    SDL_Surface* loaded_surface = IMG_Load(path);
    SDL_err_check_null(loaded_surface);

    // Convert surface to screen format
    new_texture = SDL_CreateTextureFromSurface(renderer, loaded_surface);
    SDL_err_check_null(new_texture);

    // Get rid of old loaded surface
    SDL_FreeSurface(loaded_surface);

    return new_texture;
}