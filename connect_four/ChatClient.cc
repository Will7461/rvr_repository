#include <thread>
#include "Chat.h"
#include "SDLGame.h"

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
    std::string name  = "Connect4 Client: " + std::string(argv[3]);
    SDLGame* game = new SDLGame(name, 1000, 1000);

    ChatClient ec(argv[1], argv[2], argv[3]);

    std::thread net_thread([&ec](){ ec.net_thread(); });
    net_thread.detach();

    ec.login();

    ec.input_thread();
}

