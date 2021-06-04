#include "SDLGame.h"

SDLGame::SDLGame(string winTitle, int w, int h){
    windowTitle_ = winTitle;
    width_ = w;
    height_ = h;

    initSDL();
}

SDLGame::~SDLGame(){
    closeSDL();
}

void SDLGame::initSDL(){
    int sdlInit_ret = SDL_Init(SDL_INIT_EVERYTHING);

    // Create window
	window_ = SDL_CreateWindow(windowTitle_.c_str(),
	SDL_WINDOWPOS_UNDEFINED,
	SDL_WINDOWPOS_UNDEFINED, width_, height_, SDL_WINDOW_SHOWN);

    // Create the renderer
	renderer_ = SDL_CreateRenderer(window_, -1,
    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // Clear screen (background color).
	int sdlSetDrawColor_ret = SDL_SetRenderDrawColor(renderer_, 0, 100, 100,
			255);  // Dark grey.
	int sdlRenderClear_ret = SDL_RenderClear(renderer_);
	SDL_RenderPresent(renderer_);

    // hide cursor by default
	SDL_ShowCursor(0);
}

void SDLGame::closeSDL(){
    SDL_DestroyRenderer(renderer_);
	renderer_ = nullptr;

	SDL_DestroyWindow(window_);
	window_ = nullptr;

	SDL_Quit();
}