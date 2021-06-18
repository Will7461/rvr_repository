#include "SDLGame.h"
#include <iostream>

SDLGame::SDLGame(string winTitle, int w, int h) : matrix(MATRIX_R, vector<std::pair<Vector2D, SlotState>> (MATRIX_C, std::make_pair(Vector2D(0,0),SlotState::EMPTY))) {
    windowTitle_ = winTitle;
    width_ = w;
    height_ = h;
	exit = false;

    initSDL();

	initMatrix();
}

SDLGame::~SDLGame(){
    closeSDL();
}

void SDLGame::Run(){
	while (!exit)
	{
		//Logic Update
		if(resetTableReq) resetTable();

		render();
		handleEvents();
	}
}

void SDLGame::Quit(){
	exit = true;
}

void SDLGame::putChecker(int x, int y, SlotState state){

	matrix[x][y].second = state;
	Vector2D pos = matrix[x][y].first;

	switch (state)
	{
	case SlotState::RED:{
		objects.push_back(SDLObject(pos, checker_w, checker_h, textures[TextureName::TEX_RED]));
		break;
	}
	case SlotState::YELLOW:{
		objects.push_back(SDLObject(pos, checker_w, checker_h, textures[TextureName::TEX_YELLOW]));
		break;
	}
	case SlotState::EMPTY:{
		break;
	}
	default:
		break;
	}
}

void SDLGame::resetTableRequest(){
	resetTableReq = true;
}

void SDLGame::initSDL(){
    int sdlInit_ret = SDL_Init(SDL_INIT_EVERYTHING);

    // Crear window
	window_ = SDL_CreateWindow(windowTitle_.c_str(),
	SDL_WINDOWPOS_UNDEFINED,
	SDL_WINDOWPOS_UNDEFINED, width_, height_, SDL_WINDOW_SHOWN);

    // Crear renderer
	renderer_ = SDL_CreateRenderer(window_, -1,
    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	// Carga texturas
	loadTextures();

	table = new SDLObject(Vector2D(width_ * 0.23, height_ * 0.1), 700, 600, textures[TextureName::TEX_TABLE]);

    // Esconder cursor
	// SDL_ShowCursor(0);
}

void SDLGame::initMatrix(){
	SDL_Rect r = table->getDestRect();
	Vector2D currentPos(r.x + 13,r.y + 13);

	for (int i = 0; i <  MATRIX_R; i++)
	{
		for (int j = 0; j < MATRIX_C; j++)
		{
			Vector2D* pos = &matrix[i][j].first;
			SlotState* state = &matrix[i][j].second;

			pos->x = currentPos.x;
			pos->y = currentPos.y;

			currentPos.x += 97;
		}
		currentPos.x = r.x + 13;
		currentPos.y += 96;
	}
}

void SDLGame::loadTextures(){
	for (uint i = 0; i < NUM_TEXTURES; i++) { textures[i] = new Texture(renderer_, texturesName[i]); }
}

void SDLGame::closeSDL(){
	delete table;

	for (uint i = 0; i < NUM_TEXTURES; i++) delete textures[i];

	matrix.clear();
	objects.clear();

    SDL_DestroyRenderer(renderer_);
	renderer_ = nullptr;

	SDL_DestroyWindow(window_);
	window_ = nullptr;

	SDL_Quit();
}

void SDLGame::render() const{
	SDL_SetRenderDrawColor(renderer_, 218, 112, 214, 255);

	SDL_RenderClear(renderer_);

	for (SDLObject o : objects){
		o.render();
	}

	table->render();
	
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

void SDLGame::resetTable(){
	for (int i = 0; i <  MATRIX_R; i++)
	{
		for (int j = 0; j < MATRIX_C; j++)
		{
			SlotState* state = &matrix[i][j].second;
			*state = SlotState::EMPTY; 
		}
	}
	
	objects.clear();

	resetTableReq = false;
}

void SDLObject::render() const{
	texture->render(getDestRect());
}

SDL_Rect SDLObject::getDestRect() const{
	SDL_Rect destRect;
	destRect.x = int(pos_.x);
	destRect.y = int(pos_.y);
	destRect.w = w_;
	destRect.h = h_;
	return destRect;
}