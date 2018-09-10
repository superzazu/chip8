#include <SDL.h>

#include "chip8.h"

int main(int argc, char **argv) {
    srand(time(NULL));

    if (argc != 2) {
        fprintf(stderr, "usage: chip8 <rom file>");
        return 1;
    }

    // SDL init
    if (SDL_Init(SDL_INIT_VIDEO /*| SDL_INIT_AUDIO*/) != 0) {
        SDL_Log("Unable to initialise SDL: %s", SDL_GetError());
        return 1;
    }

    // on macOS, use metal driver if available:
    // SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

    SDL_SetHint(SDL_HINT_BMP_SAVE_LEGACY_FORMAT, "1");

    // create SDL window
    SDL_Window* window = SDL_CreateWindow("chip8",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH * 10,
        SCREEN_HEIGHT * 10,
        SDL_WINDOW_RESIZABLE);

    if (window == NULL) {
        SDL_Log("Unable to create window: %s", SDL_GetError());
        return 1;
    }

    SDL_SetWindowMinimumSize(window, SCREEN_WIDTH, SCREEN_HEIGHT);

    // create renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        SDL_Log("Unable to create renderer: %s", SDL_GetError());
        return 1;
    }

    SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

    // print info on renderer:
    SDL_RendererInfo renderer_info;
    SDL_GetRendererInfo(renderer, &renderer_info);
    const bool vsync = renderer_info.flags & SDL_RENDERER_PRESENTVSYNC;
    const bool hard_accel = renderer_info.flags & SDL_RENDERER_ACCELERATED;
    SDL_Log("Using %s video renderer (vsync: %d, hardware accelerated: %d)\n",
        renderer_info.name,
        vsync,
        hard_accel);

    // create texture
    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_TARGET,
        SCREEN_WIDTH,
        SCREEN_HEIGHT);
    if (texture == NULL) {
        SDL_Log("Unable to create texture: %s", SDL_GetError());
        return 1;
    }

    // clearing texture
    SDL_SetRenderTarget(renderer, texture);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    chip8 m;
    chip8_init(&m);
    if (chip8_load_file_in_memory(&m, argv[1]) != 0) {
        return 1;
    }

    u32 timer = SDL_GetTicks();
    SDL_Event e;
    bool should_quit = false;
    while (!should_quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                should_quit = true;
            }
            else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_1: m.key_pressed[0x1] = true; break;
                    case SDLK_2: m.key_pressed[0x2] = true; break;
                    case SDLK_3: m.key_pressed[0x3] = true; break;
                    case SDLK_4: m.key_pressed[0xC] = true; break;
                    case SDLK_a: m.key_pressed[0x4] = true; break;
                    case SDLK_z: m.key_pressed[0x5] = true; break;
                    case SDLK_e: m.key_pressed[0x6] = true; break;
                    case SDLK_r: m.key_pressed[0xD] = true; break;
                    case SDLK_q: m.key_pressed[0x7] = true; break;
                    case SDLK_s: m.key_pressed[0x8] = true; break;
                    case SDLK_d: m.key_pressed[0x9] = true; break;
                    case SDLK_f: m.key_pressed[0xE] = true; break;
                    case SDLK_w: m.key_pressed[0xA] = true; break;
                    case SDLK_x: m.key_pressed[0x0] = true; break;
                    case SDLK_c: m.key_pressed[0xB] = true; break;
                    case SDLK_v: m.key_pressed[0xF] = true; break;
                }
            }
            else if (e.type == SDL_KEYUP) {
                switch (e.key.keysym.sym) {
                    case SDLK_1: m.key_pressed[0x1] = false; break;
                    case SDLK_2: m.key_pressed[0x2] = false; break;
                    case SDLK_3: m.key_pressed[0x3] = false; break;
                    case SDLK_4: m.key_pressed[0xC] = false; break;
                    case SDLK_a: m.key_pressed[0x4] = false; break;
                    case SDLK_z: m.key_pressed[0x5] = false; break;
                    case SDLK_e: m.key_pressed[0x6] = false; break;
                    case SDLK_r: m.key_pressed[0xD] = false; break;
                    case SDLK_q: m.key_pressed[0x7] = false; break;
                    case SDLK_s: m.key_pressed[0x8] = false; break;
                    case SDLK_d: m.key_pressed[0x9] = false; break;
                    case SDLK_f: m.key_pressed[0xE] = false; break;
                    case SDLK_w: m.key_pressed[0xA] = false; break;
                    case SDLK_x: m.key_pressed[0x0] = false; break;
                    case SDLK_c: m.key_pressed[0xB] = false; break;
                    case SDLK_v: m.key_pressed[0xF] = false; break;
                }
            }
        }

        // every 10ms, we step the chip8
        if (SDL_GetTicks() - timer >= 10) {
            for (int i = 0; i < 3; i++) {
                chip8_step(&m);
            }

            // drawing to screen if needed
            if (m.draw_flag) {
                m.draw_flag = false;

                SDL_SetRenderTarget(renderer, texture);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderClear(renderer);

                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                for (int i = 0; i < 2048; i++) {
                    int py = i / SCREEN_WIDTH;
                    int px = i - (py * SCREEN_WIDTH);

                    if (m.screen[i]) {
                        SDL_RenderDrawPoint(renderer, px, py);
                    }
                }
            }
        }

        SDL_SetRenderTarget(renderer, NULL);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    chip8_quit(&m);

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
