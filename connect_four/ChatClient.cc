#include <thread>
#include "Chat.h"

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

    ChatClient ec(argv[1], argv[2], argv[3]);

    std::thread net_thread([&ec](){ ec.net_thread(); });

    ec.login();

    ec.input_thread();
}

