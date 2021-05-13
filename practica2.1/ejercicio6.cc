#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

#include <iostream>
#include <thread>

#define BUFFER_SIZE 80
#define MAX_THREAD 5


//Funcion auxiliar para obtener el struct tm
struct tm* getTime(){

    time_t curr_time;
    time(&curr_time);

    return localtime(&curr_time);
}

class MessageThread
{
public:
    MessageThread(int sd) : sd_(sd) {};
    
    void do_message()
    {
        char buffer[BUFFER_SIZE];

        char host[NI_MAXHOST];
        char serv[NI_MAXSERV];

        struct sockaddr cliente;
        socklen_t clientelen = sizeof(struct sockaddr);

        bool end = false;
        while (!end)
        {
            int bytes = recvfrom(sd_, (void *) buffer, BUFFER_SIZE-1, 0, &cliente, &clientelen);

            if( bytes==-1 ){
            std::cerr << "[recvfrom]: " << strerror(errno) << '\n';
            return;
            }

            buffer[bytes] = '\0';

            std::cout << "ID: " << std::this_thread::get_id() << '\n';

            for (int i = 0; i < 10; i++)
            {
            std::cout << i << '\n';
            sleep(1);
            }

            //Obtener informacion del cliente
            int gni = getnameinfo(&cliente, clientelen, host, NI_MAXHOST, serv, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);

            if( gni != 0 ){
            std::cerr << "Error: " << gai_strerror(gni) << '\n';
            return;     
            }

            std::cout << bytes << " bytes de " << host << ":" << serv << '\n';

            int st;
            char text[BUFFER_SIZE];
            struct tm* curr_tm = getTime();

            if(curr_tm==NULL){
            std::cerr << "Error: " << strerror(errno) << '\n';
            return;
            }
            //Opcion elegida
            switch (std::tolower(buffer[0]))
            {
            case 't':
            bytes = strftime(text, BUFFER_SIZE, "%T %p", curr_tm);
            st = sendto(sd_, text, bytes, 0, &cliente, clientelen);
            break;
            case 'd':
            bytes = strftime(text, BUFFER_SIZE, "%F", curr_tm);
            st = sendto(sd_, text, bytes, 0, &cliente, clientelen);
            break;
            case 'q':
            std::cout << "Saliendo...\n";
            end = true;
            return;
            break;
            default:
            std::cout << "Comando no soportado " << buffer << '\n';
            break;
            }

            if( st==-1 ){
            std::cerr << "[sendto]: " << strerror(errno) << '\n';
            return;
            }
        }
    };
    
private:
    int sd_;
};

void trataMensaje(int sd){
    
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

    std::thread pool[MAX_THREAD];

    for (int i = 0; i < MAX_THREAD; i++)
    {
        MessageThread *mt = new MessageThread(sd);

        pool[i] = std::thread([&mt](){
            mt->do_message();

            delete mt;
        });
    }

    for(auto &thr : pool)
    {
        thr.detach();
    }

    char input;
    do{
        std::cin >> input;
    }while (std::tolower(input)!='q');

    close(sd);

    return 0;
}