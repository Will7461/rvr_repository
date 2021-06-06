#include "Chat.h"

int main(int argc, char **argv)
{
    if(argc != 3){
        std::cerr << "Usage example: " << argv[0] << " IP " << "PORT\n"; 
        return -1;
    }

    ChatServer es(argv[1], argv[2]);

    es.do_conexions();

    return 0;
}
