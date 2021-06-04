#include "SDL2/SDL.h"
#include <string>
using namespace std;

class SDLGame {
    public:
        SDLGame(SDLGame&) = delete;
        SDLGame(string winTitle, int w, int h);
        ~SDLGame();

    private:
        void initSDL();
        void closeSDL();

    SDL_Window *window_;
    SDL_Renderer *renderer_;

    string windowTitle_;
    int width_;
    int height_;
};