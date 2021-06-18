#include <string>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <map>

#include "Serializable.h"
#include "Socket.h"
#include "SDLGame.h"

#define SYNC_DELAY 0.2
#define MAX_LOBBIES 10

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 *  Mensaje del protocolo de la aplicación de Chat
 *
 *  +-------------------+
 *  | Tipo: uint8_t     | 0 (login), 1 (mensaje), 2 (logout)
 *  +-------------------+
 *  | Nick: char[8]     | Nick incluido el char terminación de cadena '\0'
 *  +-------------------+
 *  |                   |
 *  | Mensaje: char[80] | Mensaje incluido el char terminación de cadena '\0'
 *  |                   |
 *  +-------------------+
 *
 */
class Message: public Serializable
{
public:
    static const size_t MESSAGE_SIZE = sizeof(char) * 88 + sizeof(uint8_t);

    enum MessageType
    {
        LOGIN   = 0,
        MESSAGE = 1,
        LOGOUT  = 2,
        LOBBY = 3,
        PLAY = 4
    };

    Message(){};

    Message(const std::string& n, const std::string& m):nick(n),message(m){};

    void to_bin();

    int from_bin(char * bobj);

    uint8_t type;

    std::string nick;
    std::string message;
};

class LobbyMessage: public Serializable
{
public:
    static const size_t MESSAGE_SIZE = sizeof(uint8_t) + (sizeof(std::string) * (MAX_LOBBIES + 1));
    
    enum MessageType
    {
        LOBBY_REQUEST = 0, //Pide crear una lobby
        LOBBY_ACCEPT  = 1, //Respuesta positiva al intentar crear una lobby
        LOBBY_DENY = 2, //Respuesta negativa al intentar crear una lobby
        LOBBY_ASK_LIST = 3, //El cliente le pide al servidor la lista de servers
        LOBBY_SEND_LIST = 4, //El server responde con la lista de servidores
        LOBBY_JOIN_REQUEST = 5, //El cliente pide unirse a una lobby
        LOBBY_JOIN_ACCEPT = 6, //Respuesta positiva del servidor a la petición de unirse a un lobby
        LOBBY_JOIN_DENY = 7, //Respuesta negativa del servidor a la petición de unirse a un lobby
        LOBBY_START = 8, //Mensaje de empezar la partida
        LOBBY_QUIT = 9, //Uno de los jugadores abandona el lobby
        LOBBY_QUIT_REPLY = 10 //El servidor informa al otro jugador que su oponente se ha abandonado
    };

    LobbyMessage(){
        empty_list();
    };

    LobbyMessage(const std::string& lobbyN): lobbyName(lobbyN){
        empty_list();
    };

    void to_bin();

    int from_bin(char * bobj);

    void empty_list(){
        for(int i = 0; i<MAX_LOBBIES; i++){
            lobbyList[i] = "none";
        }
    }

    uint8_t type;

    std::string lobbyName;
    std::string lobbyList[MAX_LOBBIES];
};

class PlayMessage: public Serializable
{
public:
    static const size_t MESSAGE_SIZE = sizeof(uint8_t) + sizeof(std::string) + sizeof(bool) + sizeof(int) * 2;

    enum MessageType
    {
        INITIAL_TURN = 0, //Turno inicial de la partida.
        PLAY = 1 //Jugada de cada jugador
    };

    PlayMessage(){};

    PlayMessage(const std::string&l): lobbyName(l){};

    void to_bin();

    int from_bin(char * bobj);

    uint8_t type;

    std::string lobbyName;
    bool playerTurn = false;
    int posX = 0;
    int posY = 0;
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

/**
 *  Clase para el servidor de chat
 */
class ChatServer
{
public:
    ChatServer(const char * s, const char * p): socket(s, p)
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
    std::mutex clients_mtx;
    /**
     *  Lista de clientes conectados al servidor de Chat, representados por
     *  su socket
     */
    std::vector<std::unique_ptr<Socket>> clients;

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

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

/**
 *  Clase para el cliente de chat
 */
class ChatClient
{
public:
    /**
     * @param s dirección del servidor
     * @param p puerto del servidor
     * @param n nick del usuario
     */
    ChatClient(const char * s, const char * p, const char * n, SDLGame* g):socket(s, p),
        nick(n), game_(g){
            socket.connect();
        };

    /**
     *  Envía el mensaje de login al servidor
     */
    void login();

    /**
     *  Envía el mensaje de logout al servidor
     */
    void logout();

    /**
     *  Rutina principal para el Thread de E/S. Lee datos de STDIN (std::getline)
     *  y los envía por red vía el Socket.
     */
    void input_thread();

    /**
     *  Rutina del thread de Red. Recibe datos de la red y los "renderiza"
     *  en STDOUT
     */
    void net_thread();

private:

    /**
     * Socket para comunicar con el servidor
     */
    Socket socket;

    /**
     * Nick del usuario
     */
    std::string nick;

    /**
     * Lobby en la que está asignado el usuario
     */
    std::string lobbyName;

    SDLGame* game_;
};

