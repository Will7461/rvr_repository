#include "SDLGame.h"
#include "Client.h"
#include <iostream>

SDLGame::SDLGame(string winTitle, int w, int h) : matrix(MATRIX_R, vector<std::pair<Vector2D, Color>> (MATRIX_C, std::make_pair(Vector2D(0,0),Color::EMPTY))) {
    windowTitle_ = winTitle;
    width_ = w;
    height_ = h;
	exit = false;
	myTurn = false;
    removeTableReq = false;
    playing = false;
	gameEnded = false;
	
    initSDL();

	createObjects();

	initMatrix();
}

SDLGame::~SDLGame(){
    closeSDL();
}

void SDLGame::Run(){
	while (!exit)
	{
		//Logic Update
		if(removeTableReq) removeTable();
		
		render();

		if(playing) handleEvents();
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

void SDLGame::removeTableRequest(){
	removeTableReq = true;
}

void SDLGame::setClient(Client* c){
	client = c;
}

void SDLGame::gameFinished(bool won){
	gameEnded = true;
	int texW = textures[TextureName::TEX_WIN]->getW();
	int texH = textures[TextureName::TEX_WIN]->getH();
	if (endGameText) delete endGameText;
	if (won) {
		endGameText = new SDLObject(Vector2D(WINDOW_W/2 - texW / 2, WINDOW_H / 2 - texH / 2), texW, texH, textures[TextureName::TEX_WIN]);
	}
	else {
		endGameText = new SDLObject(Vector2D(WINDOW_W/2 - texW / 2, WINDOW_H / 2 - texH / 2), texW, texH, textures[TextureName::TEX_LOSE]);
	}
}

void SDLGame::startGame(bool turn){
	setTurn(turn);
	if(getTurn()) setColor(Color::RED);
	else setColor(Color::YELLOW);

	setPlaying(true);
	gameEnded = false;
}

void SDLGame::endGame(){
	setPlaying(false);
	removeTableRequest();
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
}

void SDLGame::createObjects(){
	table = new SDLObject(Vector2D(width_ * 0.23, height_ * 0.1), 700, 600, textures[TextureName::TEX_TABLE]);
	SDL_Rect r = table->getDestRect();
	arrow = new SDLObject(Vector2D(r.x + 13 + arrowLeftOffset, r.y + 13 - arrowUpOffset), arrow_size, arrow_size, textures[TextureName::TEX_ARROW]);
	titleScreen = new SDLObject(Vector2D((width_ * 0.5) - (textures[TextureName::TEX_TITLE]->getW() * 0.25), (height_ * 0.5) - (textures[TextureName::TEX_TITLE]->getH() * 0.25)),
		textures[TextureName::TEX_TITLE]->getW() * 0.5, textures[TextureName::TEX_TITLE]->getH() * 0.5, textures[TextureName::TEX_TITLE]);
	turnMarker_Player = new SDLObject(Vector2D(width_ * 0.02, height_ * 0.1), textures[TextureName::TEX_MARKER_PLAYER]->getW(), textures[TextureName::TEX_MARKER_PLAYER]->getH(), textures[TextureName::TEX_MARKER_PLAYER]);
	turnMarker_Opponent = new SDLObject(Vector2D(width_ * 0.02, height_ * 0.1), textures[TextureName::TEX_MARKER_OPPONENT]->getW(), textures[TextureName::TEX_MARKER_OPPONENT]->getH(), textures[TextureName::TEX_MARKER_OPPONENT]);
}

void SDLGame::loadTextures(){
	for (uint i = 0; i < NUM_TEXTURES; i++) { textures[i] = new Texture(renderer_, texturesName[i]); }
}

void SDLGame::closeSDL(){
	delete table;
	delete arrow;
	delete turnMarker_Player;
	delete turnMarker_Opponent;
	if (endGameText) delete endGameText;

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

	if (!playing) titleScreen->render();
	else{
		for (SDLObject o : objects){
			o.render();
		}

		table->render();
		arrow->render();
		
		if (myTurn) turnMarker_Player->render();
		else turnMarker_Opponent->render();

		if (endGameText && gameEnded) endGameText->render();
	}

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
					if (gameEnded) endGame();
					else if(myTurn) doPlay();
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

void SDLGame::removeTable(){
	for (int i = 0; i <  MATRIX_R; i++)
	{
		for (int j = 0; j < MATRIX_C; j++)
		{
			Color* state = &matrix[i][j].second;
			*state = Color::EMPTY; 
		}
	}
	removeTableReq = false;

	objects.clear();
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
	//printState();
	bool winningPlay = checkPlayerWon(i+1, currentArrowPos);
	client->sendPlay(i+1, currentArrowPos, winningPlay);
}

bool SDLGame::checkPlayerWon(int x, int y){
	//Horizontal
	int numRight = numCheckersInDir(x, y, 0, 1);
	int numLeft = numCheckersInDir(x, y, 0, -1);

	if (numRight + numLeft + 1 >= 4) return true; 

	//Vertical
	int numUp = numCheckersInDir(x, y, 1, 0);
	int numDown = numCheckersInDir(x, y, -1, 0);

	if (numUp + numDown + 1 >= 4) return true;

	//Diagonal ascendiente
	int numUpRight = numCheckersInDir(x, y, 1, 1);
	int numDownLeft = numCheckersInDir(x, y, -1, -1);

	if (numUpRight + numDownLeft + 1 >= 4) return true;

	//Diagonal descendiente
	int numDownRight = numCheckersInDir(x, y, -1, 1);
	int numUpLeft = numCheckersInDir(x, y, 1, -1);

	if (numDownRight + numUpLeft + 1 >= 4) return true;

	return false;
}

void SDLGame::printState(){
	std::cout << "Estado actual de la partida:\n";
	for (int k = 0; k < MATRIX_R; k++){
		for (int l = 0; l < MATRIX_C; l++){
			if (matrix[k][l].second == Color::EMPTY) std::cout << "0 ";
			else if (matrix[k][l].second == Color::RED) std::cout << "R ";
			else std::cout << "Y ";
		}
		std::cout << "\n";
	}
}

int SDLGame::numCheckersInDir(int posX, int posY, int dirX, int dirY){
	int newPosX = posX + dirX;
	int newPosY = posY + dirY;
	if (isViable(newPosX, newPosY) && matrix[newPosX][newPosY].second == myColor) 
		return 1 + numCheckersInDir(newPosX, newPosY, dirX, dirY);
	else return 0;
}

bool SDLGame::isViable (int posX, int posY){
	return (posX > 0 && posX < MATRIX_R && posY > 0 && posY < MATRIX_C);
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