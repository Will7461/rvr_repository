#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

#include <iostream>

#define BUFFER_SIZE 80

int main (int argc, char** argv){
    if(argc!=4){
        std::cerr << "Usage: " << argv[0] << " ip port option[t,d,q]\n";
        return 1;
    }
    struct addrinfo hints;
    struct addrinfo* res;

    memset((void * ) &hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    //Obtener info del servidor en res
    int rc = getaddrinfo(argv[1], argv[2], &hints, &res);

    if( rc != 0 ){
        std::cerr << "Error: " << gai_strerror(rc) << '\n';
        return -1;
    }

    int sd = socket(res->ai_family, res->ai_socktype, 0);

    if( sd==-1 ){
        std::cerr << "[socket]: " << strerror(errno) << '\n';
        return -1;
    }

    int c = connect(sd, res->ai_addr, res->ai_addrlen);

    if( c==-1 ){
        std::cerr << "[connect]: " << strerror(errno) << '\n';
        return -1;
    }

    
    bool waitResponse = false;
    if(std::tolower(*argv[3])=='t' || std::tolower(*argv[3])=='d') waitResponse = true;
    //Enviar comando al servidor
    int st = sendto(sd, argv[3], strlen(argv[3]), 0, res->ai_addr, res->ai_addrlen);

    freeaddrinfo(res);

    if( st==-1 ){
        std::cerr << "[sendto]: " << strerror(errno) << '\n';
        return -1;
    }
    //Espera la respuesta del servidor
    if(waitResponse){

        char buffer[BUFFER_SIZE];
        //No es necesaria la info del servidor ...NULL, NULL);
        int bytes = recvfrom(sd, (void *) buffer, BUFFER_SIZE-1, 0, NULL, NULL);

        if( bytes==-1 ){
            std::cerr << "[recvfrom]: " << strerror(errno) << '\n';
            return -1;
        }

        buffer[bytes] = '\0';

        std::cout << buffer << '\n';
    }
    
    close(sd);

    return 0;
}