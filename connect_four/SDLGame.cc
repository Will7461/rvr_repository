#include "SDLGame.h"

SDLGame::SDLGame(string winTitle, int w, int h){
    windowTitle_ = winTitle;
    width_ = w;
    height_ = h;
	exit = false;

    initSDL();
}

SDLGame::~SDLGame(){
    closeSDL();
}

void SDLGame::Run(){
	while (!exit)
	{
		//Logic Update
		
		SDL_Delay(700);
		render();
		handleEvents();
	}
}

void SDLGame::Quit(){
	exit = true;
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
	int sdlSetDrawColor_ret = SDL_SetRenderDrawColor(renderer_, 100, 149, 237, 255);  // Dark blue.
	int sdlRenderClear_ret = SDL_RenderClear(renderer_);
	SDL_RenderPresent(renderer_);

    // hide cursor by default
	// SDL_ShowCursor(0);
}

void SDLGame::closeSDL(){
    SDL_DestroyRenderer(renderer_);
	renderer_ = nullptr;

	SDL_DestroyWindow(window_);
	window_ = nullptr;

	SDL_Quit();
}

void SDLGame::render() const{
	// Randomly change the colour
	Uint8 red = rand() % 255;
	Uint8 green = rand() % 255;
	Uint8 blue = rand() % 255;
	// Fill the screen with the colour
	SDL_SetRenderDrawColor(renderer_, red, green, blue, 255);

	SDL_RenderClear(renderer_);
	
	//render de vector de texturas
	
	SDL_RenderPresent(renderer_);
}

void SDLGame::handleEvents(){
	SDL_Event event;
	while (SDL_PollEvent(&event) && !exit)
	{
		if (event.type == SDL_QUIT)
		{
			Quit();
		}
	}
}