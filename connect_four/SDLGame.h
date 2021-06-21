#pragma once
#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"
#include "Texture.h"
#include <string>
#include <vector>
#define WINDOW_W 1280
#define WINDOW_H 720
#define MATRIX_C 7
#define MATRIX_R 6

using namespace std;

const uint NUM_TEXTURES = 9;
const string texturesName[NUM_TEXTURES] = 
{"images/arrow.png", "images/red.png", "images/yellow.png", "images/table.png", "images/WinText.png", "images/LostText.png",
 "images/Titulo&Instrucciones.png", "images/TurnoJugador.png", "images/TurnoOponente.png" };
enum TextureName{
    TEX_ARROW, TEX_RED, TEX_YELLOW, TEX_TABLE, TEX_WIN, TEX_LOSE, TEX_TITLE, TEX_MARKER_PLAYER, TEX_MARKER_OPPONENT
};

enum Color{
    RED, YELLOW, EMPTY
};

const int checker_w = 85;
const int checker_h = 85;
const int arrow_size = 50;

class Vector2D{
public:
    Vector2D(uint x_, uint y_) : x(x_), y(y_){};
    uint x;
    uint y;
};

class SDLObject{
private:
	Vector2D pos_;
	uint w_, h_;
	Texture* texture;
public:
	SDLObject(Vector2D pos, uint w, uint h, Texture* texture) : pos_(pos), w_(w), h_(h), texture(texture) {};
	~SDLObject(){ texture = nullptr; };
	void render()const;
	void setPos(Vector2D newPos) {pos_ = newPos;};
    void setWH(uint w, uint h) {w_ = w; h_= h;};
	SDL_Rect getDestRect() const;
};

class Client;
class SDLGame {
public:
    SDLGame(SDLGame&) = delete;
    SDLGame(string winTitle, int w, int h);
    ~SDLGame();

    void Run();
    void Quit();

    bool getTurn() {return myTurn;};
    bool getPlaying(){return playing;};
    void setTurn(bool t) {myTurn = t;};
    void setClient(Client* c);
    void setPlaying(bool p) {playing = p;};

    void reproducePlay(int x, int y);

    void gameFinished (bool won);
    void startGame(bool turn);
    void endGame();
    void disconnect();

private:
    void initSDL();
    void initMatrix();

    void loadTextures();
    void createObjects();
    void render() const;
    void handleEvents();

    void closeSDL();

    void removeTable();
    void moveArrow(int d);
    void doPlay();
    bool checkPlayerWon(int x, int y);
    int numCheckersInDir(int posX, int posY, int dirX, int dirY);
    bool isViable (int posX, int posY);
    void printState();
    void setColor(Color c){myColor = c;};

    void putChecker(int x, int y, Color state);
    void removeTableRequest();

    Client* client;

    SDL_Window *window_;
    SDL_Renderer *renderer_;

    vector<vector<std::pair<Vector2D, Color>>> matrix;

    Texture* textures[NUM_TEXTURES];
    vector<SDLObject> objects;
    SDLObject* table;
    SDLObject* arrow;
    SDLObject* turnMarker_Player;
    SDLObject* turnMarker_Opponent;
    SDLObject* endGameText;
    SDLObject* titleScreen;
    Color myColor;

    string windowTitle_;
    const int arrowUpOffset = 75;
    const int arrowLeftOffset = 16;
    int currentArrowPos = 0;
    int width_;
    int height_;
    bool exit;
    bool myTurn;
    bool removeTableReq;
    bool playing;
    bool gameEnded;
};