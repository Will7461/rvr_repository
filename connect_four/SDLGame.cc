#include "SDLGame.h"
#include "Client.h"
#include <iostream>

SDLGame::SDLGame(string winTitle, int w, int h) : matrix(MATRIX_R, vector<std::pair<Vector2D, Color>> (MATRIX_C, std::make_pair(Vector2D(0,0),Color::EMPTY))) {
    windowTitle_ = winTitle;
    width_ = w;
    height_ = h;
	exit = false;
	myTurn = false;
    resetTableReq = false;
    playing = false;

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
		
		if(playing){
			render();
			handleEvents();
		}
	}
}

void SDLGame::Quit(){
	exit = true;
}

void SDLGame::putChecker(int x, int y, Color state){

	matrix[x][y].second = state;
	Vector2D pos = matrix[x][y].first;

	switch (state)
	{
	case Color::RED:{
		objects.push_back(SDLObject(pos, checker_w, checker_h, textures[TextureName::TEX_RED]));
		break;
	}
	case Color::YELLOW:{
		objects.push_back(SDLObject(pos, checker_w, checker_h, textures[TextureName::TEX_YELLOW]));
		break;
	}
	case Color::EMPTY:{
		break;
	}
	default:
		break;
	}
}

void SDLGame::reproducePlay(int x, int y){
	Color otherColor = (myColor==Color::YELLOW) ? Color::RED : Color::YELLOW;
	if(myTurn) putChecker(x,y,myColor);
	else putChecker(x,y,otherColor);
}

void SDLGame::resetTableRequest(){
	resetTableReq = true;
}

void SDLGame::setClient(Client* c){
	client = c;
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

			pos->x = currentPos.x;
			pos->y = currentPos.y;

			currentPos.x += 97;
		}
		currentPos.x = r.x + 13;
		currentPos.y += 96;
	}

	arrow = new SDLObject(Vector2D(matrix[0][0].first.x + arrowLeftOffset, matrix[0][0].first.y - arrowUpOffset), arrow_size, arrow_size, textures[TextureName::TEX_ARROW]);
}

void SDLGame::loadTextures(){
	for (uint i = 0; i < NUM_TEXTURES; i++) { textures[i] = new Texture(renderer_, texturesName[i]); }
}

void SDLGame::closeSDL(){
	delete table;
	delete arrow;

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
	arrow->render();
	
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
		else if(event.type == SDL_KEYDOWN){
			switch (event.key.keysym.sym) {
				case SDLK_SPACE:
					if(myTurn) doPlay();
					break;
				case SDLK_LEFT:
					moveArrow(-1);
					break;
				case SDLK_RIGHT:
					moveArrow(1);
					break;
				case SDLK_ESCAPE:
					Quit();
					break;
				}
		}
	}
}

void SDLGame::resetTable(){
	for (int i = 0; i <  MATRIX_R; i++)
	{
		for (int j = 0; j < MATRIX_C; j++)
		{
			Color* state = &matrix[i][j].second;
			*state = Color::EMPTY; 
		}
	}
	
	objects.clear();
	render();
	resetTableReq = false;
}

void SDLGame::moveArrow(int d){
	if(d<0){
		currentArrowPos--;
		if(currentArrowPos<0) currentArrowPos = MATRIX_C-1;
		arrow->setPos(Vector2D(matrix[0][currentArrowPos].first.x + arrowLeftOffset, matrix[0][currentArrowPos].first.y - arrowUpOffset));
	}
	else if(d>0){
		currentArrowPos++;
		currentArrowPos%=MATRIX_C;
		arrow->setPos(Vector2D(matrix[0][currentArrowPos].first.x + arrowLeftOffset, matrix[0][currentArrowPos].first.y - arrowUpOffset));
	}
}

void SDLGame::doPlay(){
	if(matrix[0][currentArrowPos].second != Color::EMPTY) return;

	int i = MATRIX_R-1;
	Color freeSlot;
	do{
		freeSlot = matrix[i][currentArrowPos].second;
		i--;
	}while (i>=0 && freeSlot != Color::EMPTY);
	
	client->sendPlay(i+1, currentArrowPos);
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