#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"
#include "Texture.h"
#include <string>
using namespace std;

const uint NUM_TEXTURES = 2;
const string textName[NUM_TEXTURES] = {"..\\images\\bg1.png", "..\\images\\bg2.png" };

class Vector2D{
public:
    uint x;
    uint y;
};

class SDLGame {
public:
    SDLGame(SDLGame&) = delete;
    SDLGame(string winTitle, int w, int h);
    ~SDLGame();

    void Run();
    void Quit();
    bool getTurn() {return myTurn;}
    void setTurn(bool t) {myTurn = t;}

private:
    void initSDL();
    void closeSDL();
    void render() const;
    void handleEvents();

    SDL_Window *window_;
    SDL_Renderer *renderer_;

    string windowTitle_;
    int width_;
    int height_;
    bool exit;
    bool myTurn = false;
};

class SDLObject{
private:
	Vector2D pos_;
	uint w_, h_;
	Texture* texture;
public:
	SDLObject(Vector2D pos, uint w, uint h, Texture* texture) : pos_(pos), w_(w), h_(h), texture(texture) {};
	~SDLObject(){};
	void render()const;
	void updatePos(Vector2D newPos) {pos_ = newPos;};
	SDL_Rect getDestRect() const;
};