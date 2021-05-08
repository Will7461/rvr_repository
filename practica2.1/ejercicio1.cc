#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

#include <iostream>

int main (int argc, char** argv){
    struct addrinfo hints;
    struct addrinfo* res;

    memset((void * ) &hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = 0;

    int rc = getaddrinfo(argv[1], NULL, &hints, &res);

    if( rc != 0 ){
        std::cerr << "Error: Name or service not known" << '\n';
        return -1;
    }

    for(auto i = res; i != nullptr; i = i->ai_next){
        char host[NI_MAXHOST];

        getnameinfo(i->ai_addr, i->ai_addrlen, host, NI_MAXHOST, NULL, NI_MAXSERV, NI_NUMERICHOST);
        std::cout << host << " " << i->ai_family << " " << i->ai_socktype << '\n';
    }

    freeaddrinfo(res);
}