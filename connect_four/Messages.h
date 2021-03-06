#include <string>
#include <unistd.h>
#include <string.h>

#include "Serializable.h"

#define MAX_LOBBIES 10

#define RED_COLOR "\033[1;31m"
#define GREEN_COLOR "\033[1;32m"
#define YELLOW_COLOR "\033[1;33m"
#define BLUE_COLOR "\033[1;34m"
#define MAGENTA_COLOR "\033[1;35m"
#define CYAN_COLOR "\033[1;36m"
#define RESET_COLOR "\033[0m"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

class Message: public Serializable
{
public:
    static const size_t MESSAGE_SIZE = (sizeof(uint8_t) + sizeof(char) * 88) + (sizeof(std::string) * (MAX_LOBBIES + 1) + sizeof(bool) * 2 + sizeof(int) * 2);

    enum MessageType
    {
        LOGIN   = 0,
        MESSAGE = 1,
        LOGOUT  = 2,

        LOBBY_REQUEST, //Pide crear una lobby
        LOBBY_ACCEPT, //Respuesta positiva al intentar crear una lobby
        LOBBY_DENY, //Respuesta negativa al intentar crear una lobby
        LOBBY_ASK_LIST, //El cliente le pide al servidor la lista de servers
        LOBBY_SEND_LIST, //El server responde con la lista de servidores
        LOBBY_JOIN_REQUEST, //El cliente pide unirse a una lobby
        LOBBY_JOIN_DENY, //Respuesta negativa del servidor a la petición de unirse a un lobby
        LOBBY_START, //Mensaje de empezar la partida
        LOBBY_QUIT, //Uno de los jugadores abandona el lobby
        LOBBY_QUIT_REPLY, //El servidor informa al otro jugador que su oponente se ha abandonado

        INITIAL_TURN, //Turno inicial de la partida.
        PLAYER_PLAY //Jugada de cada jugador
    };

    Message(){ empty_list();};

    Message(const std::string& n, const std::string& m, const std::string& l):nick(n),message(m), lobbyName(l){ empty_list(); };

    void to_bin();

    int from_bin(char * bobj);

    void empty_list(){
        for(int i = 0; i<MAX_LOBBIES; i++){
            lobbyList[i] = "none";
        }
    }

    uint8_t type;

    std::string nick;
    std::string message;

    std::string lobbyName;
    std::string lobbyList[MAX_LOBBIES];

    bool playerTurn = false;
    bool playerWon = false;
    int posX = 0;
    int posY = 0;
};
