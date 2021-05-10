#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

#include <iostream>

#define BUFFER_SIZE 80
//Funcion auxiliar para obtener el struct tm
struct tm* getTime(){

    time_t curr_time;
    time(&curr_time);

    return localtime(&curr_time);
}

int main (int argc, char** argv){
    if(argc!=3){
        std::cerr << "Usage: " << argv[0] << " ip port\n";
        return 1;
    }

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

    bool end = false;
    //Bucle principal
    while(!end){
        char buffer[BUFFER_SIZE];

        char host[NI_MAXHOST];
        char serv[NI_MAXSERV];

        struct sockaddr cliente;
        socklen_t clientelen = sizeof(struct sockaddr);

        int bytes = recvfrom(sd, (void *) buffer, BUFFER_SIZE-1, 0, &cliente, &clientelen);

        if( bytes==-1 ){
            std::cerr << "[recvfrom]: " << strerror(errno) << '\n';
            return -1;
        }

        buffer[bytes] = '\0';
        //Obtener informacion del cliente
        int gni = getnameinfo(&cliente, clientelen, host, NI_MAXHOST, serv, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);

        if( gni != 0 ){
            std::cerr << "Error: " << gai_strerror(gni) << '\n';
            return -1;        
        }

        std::cout << bytes << " bytes de " << host << ":" << serv << '\n';

        int st;
        char text[BUFFER_SIZE];
        struct tm* curr_tm = getTime();

        if(curr_tm==NULL){
            std::cerr << "Error: " << strerror(errno) << '\n';
            return -1;
        }
        //Opcion elegida
        switch (std::tolower(buffer[0]))
        {
        case 't':
            bytes = strftime(text, BUFFER_SIZE, "%T %p", curr_tm);
            st = sendto(sd, text, bytes, 0, &cliente, clientelen);
            break;
        case 'd':
            bytes = strftime(text, BUFFER_SIZE, "%F", curr_tm);
            st = sendto(sd, text, bytes, 0, &cliente, clientelen);
            break;
        case 'q':
            std::cout << "Saliendo...\n";
            end = true;
            break;
        default:
            std::cout << "Comando no soportado " << buffer << '\n';
            break;
        }

        if( st==-1 ){
            std::cerr << "[sendto]: " << strerror(errno) << '\n';
            return -1;
        }

    }

    close(sd);

    return 0;
}