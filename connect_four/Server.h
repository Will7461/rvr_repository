#include <vector>
#include <thread>
#include <mutex>
#include <map>

#include "Socket.h"
#include "Messages.h"

#define MAX_LOBBIES 10

/**
 *  Clase para el servidor de chat
 */
class Server
{
public:
    Server(const char * s, const char * p): socket(s, p)
    {
        socket.bind();
        socket.listen(16);
    };

    /**
     *  Thread principal del servidor recibe mensajes en el socket y
     *  lo distribuye a los clientes. Mantiene actualizada la lista de clientes
     */
    void do_conexions();

private:
    std::mutex lobbies_mtx;
    /**
     *  Map de lobbies<Lobby Name,Lobby Full>
     */
    std::map<std::string, std::pair<Socket*,Socket*>> lobbies;

    /**
     * Socket del servidor
     */
    Socket socket;
};

