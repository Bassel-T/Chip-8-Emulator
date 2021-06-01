#include "chip8.h"
#include <iostream>
#include "SDL2/SDL.h"
#include <stdio.h>
#include "stdint.h"
#include <thread>
#include <chrono>

using namespace std;

// Mapping the keys
unsigned char keymap[16] = {
    SDLK_x,
    SDLK_1,
    SDLK_2,
    SDLK_3,
    SDLK_q,
    SDLK_w,
    SDLK_e,
    SDLK_a,
    SDLK_s,
    SDLK_d,
    SDLK_z,
    SDLK_c,
    SDLK_4,
    SDLK_r,
    SDLK_f,
    SDLK_v
};

chip8 myChip;

void setKeys() {
    // Check for an event
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        // Closing the window
        if (event.type == SDL_QUIT) {
            exit(0);
        }

        if (event.type == SDL_KEYDOWN) {
            // Some button was held down
            for (int i = 0; i < 16; ++i) {
                if (event.key.keysym.sym == keymap[i]) {
                    myChip.key[i] = 1;
                }
            }
        }
            
        if (event.type == SDL_KEYUP) {
            // Some button was released
            for (int i = 0; i < 16; ++i) {
                if (event.key.keysym.sym == keymap[i]) {
                    myChip.key[i] = 0;
                }
            }
        }            
    }
}

int main(int argc, char* argv[]) {
    
    if (argc != 2) {
        // Invalid argument count
        cout << "Correct usage:" << endl << argv[0] << " <filename>" << endl;
        return 1;
    }

    // Initialize the system and load the game
    myChip.initialize();
    
    // Try to load the game
    if (!myChip.loadGame(argv[1])) {
        cout << "Error loading game " << argv[1] << "!" << endl;
        return 2;
    }

    // Change the window size
    int w = 640;
    int h = 320;

    // SDL Setup taken from github.com/AkshatSharma05/CHIP-8/blob/master/chip8.cpp
    SDL_Init(SDL_INIT_EVERYTHING);

    // Generate the window
    SDL_Window *window = NULL;
    window = SDL_CreateWindow(argv[1],
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        w, h, SDL_WINDOW_SHOWN);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_RenderSetLogicalSize(renderer, w, h);

    SDL_Texture *tex = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        64, 32);

    // Take care of the pixels to be displayed
    unsigned int pixels[2048];

    // Emulation loop
    while (true) {
        // Emulate one cycle
        myChip.emulateCycle();

        setKeys();

        // If the draw flag is set, update the screen
        if (myChip.drawFlag) {            
            for (int i = 0; i < 2048; ++i) {
                // Pixels are in the hex form #ARGB
                pixels[i] = (0x00FFFFFF * myChip.gfx[i]) | 0xFF000000;
            }

            // Update the screen
            SDL_UpdateTexture(tex, NULL, pixels, 64 * sizeof(unsigned int));
            SDL_RenderCopy(renderer, tex, NULL, NULL);
            SDL_RenderPresent(renderer);
            myChip.drawFlag = false;
        }

        // Limit the framerate
        std::this_thread::sleep_for(std::chrono::microseconds(1200));

    }

    return 0;
}