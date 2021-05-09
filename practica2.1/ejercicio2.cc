#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

#include <iostream>

int main (int argc, char** argv){
    struct addrinfo hints;
    struct addrinfo* res;

    memset((void * ) &hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    int rc = getaddrinfo(argv[1], argv[2], &hints, &res);

    if( rc != 0 ){
        std::cerr << "Error: " << gai_strerror(rc) << '\n';
        return -1;
    }

    // for(auto i = res; i != nullptr; i = i->ai_next){
    //     char host[NI_MAXHOST];

    //     getnameinfo(i->ai_addr, i->ai_addrlen, host, NI_MAXHOST, NULL, NI_MAXSERV, NI_NUMERICHOST);
    //     std::cout << host << " " << i->ai_family << " " << i->ai_socktype << '\n';
    // }

    int sd = socket(res->ai_family, res->ai_socktype, 0);

    if( sd==-1 ){
        std::cerr << "[socket]: " << strerror(errno) << '\n';
        return -1;
    }

    int b = bind(sd, res->ai_addr, res->ai_addrlen);

    if( b==-1 ){
        std::cerr << "[bind]: " << strerror(errno) << '\n';
        return -1;
    }

    freeaddrinfo(res);

    while(true){
        char buffer[80];

        char host[NI_MAXHOST];
        char serv[NI_MAXSERV];

        struct sockaddr cliente;
        socklen_t clientelen = sizeof(struct sockaddr);

        int bytes = recvfrom(sd, (void *) buffer, 80, 0, &cliente, &clientelen);

        if( bytes==-1 ){
            std::cerr << "[recvfrom]: " << strerror(errno) << '\n';
            return -1;
        }

        int gni = getnameinfo(&cliente, clientelen, host, NI_MAXHOST, serv, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);

        if( gni != 0 ){
            std::cerr << "Error: " << gai_strerror(gni) << '\n';
            return -1;        
        }

        std::cout << "Host: " << host << " Port: " << serv << '\n';
        std::cout << "\tData: " << buffer << '\n';

    }

    return 0;
}