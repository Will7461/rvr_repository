#include <string>
#include <unistd.h>
#include <string.h>


#include "Serializable.h"

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
        PLAYER_PLAY = 1 //Jugada de cada jugador
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

