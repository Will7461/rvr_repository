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

    int c = connect(sd, res->ai_addr, res->ai_addrlen);

    freeaddrinfo(res);

    if( c==-1 ){
        std::cerr << "[connect]: " << strerror(errno) << '\n';
        return -1;
    }

    //Bucle principal
    while(true){
        char auxBuff[BUFFER_SIZE];
        std::cin >> auxBuff;
        int nRead = strlen(auxBuff);
        auxBuff[nRead] = '\0';

        int s = send(sd, auxBuff, nRead+1, 0);

        if( s==-1 ){
            std::cerr << "[send]: " << strerror(errno) << '\n';
            return -1;
        }

        if(*auxBuff=='Q') break;

        char buffer[BUFFER_SIZE];

        int bytes = recv(sd, (void *) buffer, BUFFER_SIZE-1, 0);

        if( bytes==-1 ){
            std::cerr << "[recv]: " << strerror(errno) << '\n';
            return -1;
        }

        buffer[bytes] = '\0';

        std::cout << buffer << '\n';
    }

    close(sd);

    return 0;
}