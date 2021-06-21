#include "Server.h"

int main(int argc, char **argv)
{
    if(argc != 3){
        std::cerr << "Usage example: " << argv[0] << " IP " << "PORT\n"; 
        return -1;
    }

    srand(time(NULL)); //Usamos numeros aleatorios para decidir que jugador empieza la partida.
    Server es(argv[1], argv[2]);

    std::cout << MAGENTA_COLOR <<"===[SERVIDOR INICIADO]===" << RESET_COLOR << '\n';

    std::thread net_thread([&es](){ es.do_conexions(); });
    net_thread.detach();

    sleep(10000);

    return 0;
}
