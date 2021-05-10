#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

#include <iostream>

#define BUFFER_SIZE 80

int main (int argc, char** argv){
    if(argc!=3){
        std::cerr << "Usage: " << argv[0] << " ip port\n";
        return 1;
    }

    struct addrinfo hints;
    struct addrinfo* res;

    memset((void * ) &hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

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

    int b = bind(sd, res->ai_addr, res->ai_addrlen);

    freeaddrinfo(res);

    if( b==-1 ){
        std::cerr << "[bind]: " << strerror(errno) << '\n';
        return -1;
    }

    int l = listen(sd, 16);

    if( l==-1 ){
        std::cerr << "[listen]: " << strerror(errno) << '\n';
        return -1;
    }

    struct sockaddr cliente;
    socklen_t clientelen = sizeof(struct sockaddr);

    int cliente_sd = accept(sd, &cliente, &clientelen);

    if( cliente_sd==-1 ){
        std::cerr << "[accept]: " << strerror(errno) << '\n';
        return -1;
    }

    char host[NI_MAXHOST];
    char serv[NI_MAXSERV];
    //Obtener informacion del cliente
    int gni = getnameinfo(&cliente, clientelen, host, NI_MAXHOST, serv, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);

    if( gni != 0 ){
        std::cerr << "Error: " << gai_strerror(gni) << '\n';
        return -1;        
    }

    std::cout << "Conexión desde " << host << " " << serv << '\n';

    //Bucle principal
    while(true){
        char buffer[BUFFER_SIZE];

        int bytes = recv(cliente_sd, (void *) buffer, BUFFER_SIZE-1, 0);

        if( bytes==0 ){
            std::cout << "Conexión terminada\n";
            break;
        }

        if( bytes==-1 ){
            std::cerr << "[recv]: " << strerror(errno) << '\n';
            return -1;
        }

        buffer[bytes] = '\0';

        int s = send(cliente_sd, buffer, bytes, 0);

        if( s==-1 ){
            std::cerr << "[send]: " << strerror(errno) << '\n';
            return -1;
        }
    }

    close(sd);

    return 0;
}