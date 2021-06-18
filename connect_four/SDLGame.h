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

const uint NUM_TEXTURES = 3;
const string texturesName[NUM_TEXTURES] = {"images/red.png", "images/yellow.png", "images/table.png" };
enum TextureName{
    TEX_RED, TEX_YELLOW, TEX_TABLE
};

enum SlotState{
    RED, YELLOW, EMPTY
};

const int checker_w = 85;
const int checker_h = 85;

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

class SDLGame {
public:
    SDLGame(SDLGame&) = delete;
    SDLGame(string winTitle, int w, int h);
    ~SDLGame();

    void Run();
    void Quit();
    bool getTurn() {return myTurn;}
    void setTurn(bool t) {myTurn = t;}
    void putChecker(int x, int y, SlotState state);
    void resetTableRequest();

private:
    void initSDL();
    void initMatrix();
    void loadTextures();
    void closeSDL();
    void render() const;
    void handleEvents();
    void resetTable();

    SDL_Window *window_;
    SDL_Renderer *renderer_;

    vector<vector<std::pair<Vector2D, SlotState>>> matrix;

    Texture* textures[NUM_TEXTURES];
    vector<SDLObject> objects;
    SDLObject* table;

    string windowTitle_;
    int width_;
    int height_;
    bool exit;
    bool myTurn = false;
    bool resetTableReq = false;
};