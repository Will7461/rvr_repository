#include <thread>
#include "Chat.h"
#include "SDL2/SDL.h"

const int WIN_WIDTH = 300;
const int WIN_HEIGTH = 300;

int main(int argc, char **argv)
{
    if(argc != 4){
        std::cerr << "Usage example: " << argv[0] << " IP " << "PORT " << "NICKNAME\n"; 
        return -1;
    }
    if(std::string(argv[3]).length() > 8){
        std::cerr << "Nombre demasiado largo\n";
        return -1;
    }

    int sdlInit_ret = SDL_Init(SDL_INIT_EVERYTHING);

	// Create window
	SDL_Window *window_ = SDL_CreateWindow("myGame",
	SDL_WINDOWPOS_UNDEFINED,
    SDL_WINDOWPOS_UNDEFINED, WIN_WIDTH, WIN_HEIGTH, SDL_WINDOW_SHOWN);

    ChatClient ec(argv[1], argv[2], argv[3]);

    std::thread net_thread([&ec](){ ec.net_thread(); });

    ec.login();

    ec.input_thread();
}

