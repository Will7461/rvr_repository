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

		handleEvents();
	}
}

void SDLGame::Quit(){
	exit = true;
}

/**
 * Pinta una textura de una ficha en una posición concreta.
 * Param x: Posición x de la ficha a colocar.
 * Param y: Posición y de la ficha a colocar.
 * Param state: Jugador al que pertenece (Red, yellow, empty)
 */
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

/**
 * Coloca una ficha de un jugador en una posición concreta.
 * Este método se llama al recibir la jugada de un jugador.
 * Param x: Posición en x de la ficha.
 * Param y: Posición en y de la ficha.
 */
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

/**
 * Establece el estado del juego como acabado y pinta un texto de victoria/derrota.
 * Param won: Indica si el jugador ha ganado o ha perdido.
 */
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

/**
 * Establece el color y el turno del jugador.
 * Param turn: Indica si el jugador empieza la partida o va segundo.
 */
void SDLGame::startGame(bool turn){
	setTurn(turn);
	if(getTurn()) setColor(Color::RED);
	else setColor(Color::YELLOW);

	setPlaying(true);
	gameEnded = false;
}

/**
 * Acaba la partida.
 */
void SDLGame::endGame(){
	setPlaying(false);
	removeTableRequest();
}

/**
 * Inicialización de recursos de SDL
 */
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

/**
 * Define la posición global de cada casilla en el tablero.
 * Las posiciones se guardan en una matriz desde la que se accederá desde otros métodos.
 */
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

/**
 * Crea objetos que se usarán a lo largo de toda la partida.
 */
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

	if(matrix.size()>0) matrix.clear();
	if(objects.size()>0) objects.clear();

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
		//Abandona el lobby y cierra el juego.
		if (event.type == SDL_QUIT)
		{
			if(playing) client->leaveLobby();
			client->logout();
			Quit();
		}
		else if(event.type == SDL_KEYDOWN){

			switch (event.key.keysym.sym) {
				case SDLK_ESCAPE:
					if(playing) client->leaveLobby();
					client->logout();
					Quit();
					break;
			}

			if(playing){
				switch (event.key.keysym.sym) {
				case SDLK_SPACE:
					//Vuelve al menú principal.
					if (gameEnded){
						endGame();
						client->leaveLobby();
					}
					//Pone una ficha donde esté la flecha.
					else if(myTurn) doPlay();
					break;
				//Mueve la flecha para dejar una casilla.
				case SDLK_LEFT:
					moveArrow(-1);
					break;
				case SDLK_RIGHT:
					moveArrow(1);
					break;
				}
			}
		}
	}
}

/**
 * Borra las fichas de los jugadores.
 */
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
/**
 * Mueve la flecha para colocar las fichas.
 * Param d: Dirección izquierda o derecha
 */
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

/**
 * Coloca una ficha en la posición de la flecha.
 */
void SDLGame::doPlay(){
	if(matrix[0][currentArrowPos].second != Color::EMPTY) return;

	int i = MATRIX_R-1; //Altura del tablero
	Color freeSlot;
	do{ //La ficha cae en la última posición libre en su columna.
		freeSlot = matrix[i][currentArrowPos].second;
		i--;
	}while (i>=0 && freeSlot != Color::EMPTY);

	bool winningPlay = checkPlayerWon(i+1, currentArrowPos);
	client->sendPlay(i+1, currentArrowPos, winningPlay);
}

/**
 * Calcula si el jugador ha ganado la partida.
 * Param x: Posición en x de la ficha que acaba de colocar.
 * Param y: Posición en y de la ficha que acaba de colocar.
 */
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

/**
 * Imprime en consola el estado de la partida. Método para debuggear.
 */
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

/**
 * Función recursiva que devuelve el número de casillas consecutivas de un jugador en una dirección concreta.
 * Param posX: Posición en x de la casilla.
 * Param posY: Posición en y de la casilla.
 * Param dirX: Dirección en x en la que se está comprobando.
 * Param dirY: Dirección en y en la que se está comprobando.
 */
int SDLGame::numCheckersInDir(int posX, int posY, int dirX, int dirY){
	//Obtiene la siguiente ficha.
	int newPosX = posX + dirX;
	int newPosY = posY + dirY;
	//Si la posición de la ficha es viable y pertenece al jugador, analizamos la siguiente ficha.
	if (isViable(newPosX, newPosY) && matrix[newPosX][newPosY].second == myColor) 
		return 1 + numCheckersInDir(newPosX, newPosY, dirX, dirY);
	else return 0;
}

/**
 * Devuelve si la posición de una ficha es viable (No se sale del tablero)
 */
bool SDLGame::isViable (int posX, int posY){
	return (posX >= 0 && posX < MATRIX_R && posY >= 0 && posY < MATRIX_C);
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