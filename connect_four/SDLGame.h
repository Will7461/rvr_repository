#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"
#include <string>
using namespace std;

class SDLGame {
public:
    SDLGame(SDLGame&) = delete;
    SDLGame(string winTitle, int w, int h);
    ~SDLGame();

    void Run();
    void Quit();

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
};