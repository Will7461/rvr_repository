#include <string.h>

#include "Serializable.h"
#include "Socket.h"

Socket::Socket(const char * address, const char * port):sd(-1)
{
    //Construir un socket de tipo AF_INET y SOCK_STREAM usando getaddrinfo.
    struct addrinfo hints;
    struct addrinfo* res;

    memset((void * ) &hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int rc = getaddrinfo(address, port, &hints, &res);

    if( rc != 0 ){
        std::cerr << "Error: " << gai_strerror(rc) << '\n';
        return;
    }

    int soc = socket(res->ai_family, res->ai_socktype, 0);

    if( soc==-1 ){
        std::cerr << "[socket]: " << strerror(errno) << '\n';
        return;
    }

    //Con el resultado inicializar los miembros sd, sa y sa_len de la clase
    sd = soc;
    sa = *(res->ai_addr);
    sa_len = res->ai_addrlen;

    freeaddrinfo(res);
}

int Socket::recv(Serializable &obj, Socket * &sock)
{
    struct sockaddr sa;
    socklen_t sa_len = sizeof(struct sockaddr);

    char buffer[MAX_MESSAGE_SIZE];

    ssize_t bytes = ::recv(sd, buffer, MAX_MESSAGE_SIZE, 0);

    if ( bytes <= 0 )
    {
        return -1;
    }

    if ( sock != 0 )
    {
        sock = new Socket(&sa, sa_len);
    }

    obj.from_bin(buffer);

    return 0;
}

int Socket::send(Serializable& obj)
{
    //Serializar el objeto
    obj.to_bin();
    //Enviar el objeto binario a sock usando el socket sd
    int s = ::send(sd, obj.data(), obj.size(), 0);

    return (s>=0) ? 0 : -1;
}

int Socket::send(Serializable& obj, const Socket& sock)
{
    //Serializar el objeto
    obj.to_bin();
    //Enviar el objeto binario a sock usando el socket sd
    int s = ::send(sock.sd, obj.data(), obj.size(), 0);

    return (s>=0) ? 0 : -1;
}

int Socket::accept(struct sockaddr& client, socklen_t& clientelen){

    int cliente_sd = ::accept(sd, &client, &clientelen);

    if( cliente_sd==-1 ){
    std::cerr << "[accept]: " << strerror(errno) << '\n';
    return -1;
    }

    char host[NI_MAXHOST];
    char serv[NI_MAXSERV];
    //Obtener informacion del cliente
    int gni = getnameinfo(&client, clientelen, host, NI_MAXHOST, serv, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);

    if( gni != 0 ){
    std::cerr << "Error: " << gai_strerror(gni) << '\n';
    return -1;        
    }

    return cliente_sd;
}

bool operator== (const Socket &s1, const Socket &s2)
{
    //Comparar los campos sin_family, sin_addr.s_addr y sin_port
    //de la estructura sockaddr_in de los Sockets s1 y s2
    //Retornar false si alguno difiere
    sockaddr_in* s1Sa = (sockaddr_in*) &s1.sa;
    sockaddr_in* s2Sa = (sockaddr_in*) &s2.sa;

    return (s1Sa->sin_family == s2Sa->sin_family &&
    s1Sa->sin_addr.s_addr == s2Sa->sin_addr.s_addr &&
    s1Sa->sin_port == s2Sa->sin_port);
};

std::ostream& operator<<(std::ostream& os, const Socket& s)
{
    char host[NI_MAXHOST];
    char serv[NI_MAXSERV];

    getnameinfo((struct sockaddr *) &(s.sa), s.sa_len, host, NI_MAXHOST, serv,
                NI_MAXSERV, NI_NUMERICHOST);

    os << host << ":" << serv;

    return os;
};

