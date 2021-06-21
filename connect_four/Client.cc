#include "Client.h"
#include "SDLGame.h"
#include <time.h>
#include <stdlib.h>
#include <algorithm>
#include <cctype>

Client::Client(const char * s, const char * p, const char * n, SDLGame* g) : socket(s, p),
nick(n), game_(g), lobbyName(""){
    socket.connect();
    game_->setClient(this);
}

/**
 * Envia un mensaje al servidor para que le registre.
 */
void Client::login()
{
    std::string msg;

    Message em(nick, msg, "");
    em.type = Message::LOGIN;

    socket.send(em);
}

/**
 * Desconecta al cliente con el server.
 */
void Client::logout()
{
    Message em(nick, "", "");
    em.type = Message::LOGOUT;

    socket.send(em);
}

/**
 * El cliente abandona la sala y se lo coumunica al cliente. También cierra el juego.
 */
void Client::leaveLobby(){
    std::cout << YELLOW_COLOR << "[SALIENDO DE LOBBY]" << RESET_COLOR << '\n';

    Message em(nick,"",lobbyName);
    em.type = Message::LOBBY_QUIT;
    socket.send(em);
    lobbyName = "";

    game_->endGame();
}

std::string Client::toLower(std::string const& s){
    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(),
    [](unsigned char c){ return std::tolower(c); });
    return lower;
}
/**
 * Hilo por el que el cliente envía mensajes al servidor dependiendo del input que se le proporcione.
 */
void Client::input_thread()
{
    while (true)
    {
        std::string msg;
        std::getline(std::cin, msg);
        if(msg =="!q" || toLower(msg)=="quit"){
            if(game_->getPlaying()) leaveLobby();
            logout();
            std::cerr << RED_COLOR << "[SALIENDO DEL JUEGO]" << RESET_COLOR << '\n';
            break;
        }
        else if(msg.length()>80){
            std::cerr << "Mensaje demasiado largo para ser enviado\n";
            continue;
        }

        if(lobbyName!=""){
            if(toLower(msg) == "abandon"){
                leaveLobby();
            }
            else{
                Message em(nick,msg,lobbyName);
                em.type = Message::MESSAGE;
                socket.send(em);
            }
        }
        else{
            if(toLower(msg) == "create"){
            std::cout << "Introduce nombre de lobby: ";
            std::string lName;
            std::getline(std::cin, lName);

            Message em(nick,msg,lName);
            em.type = Message::LOBBY_REQUEST;

            socket.send(em);
            }
            else if(toLower(msg) == "list"){

                Message em(nick,msg,"");
                em.type = Message::LOBBY_ASK_LIST;

                socket.send(em);
            }
            else if(toLower(msg) == "join"){
                std::cout << "Introduce nombre de lobby al que quieres unirte: ";
                std::string lName;
                std::getline(std::cin, lName);

                Message em(nick,msg,lName);
                em.type = Message::LOBBY_JOIN_REQUEST;
                socket.send(em);
            }
            else{
                std::cout << RED_COLOR << "[COMANDO NO RECONOCIDO]" << RESET_COLOR << '\n';
            }
        }
    }
}


/**
 * Hilo por el que el cliente escucha al servidor y procesa sus mensajes
 */
void Client::net_thread()
{
    while(true)
    {
        //Recibir Mensajes de red
        Message ms;

        int r = socket.recv(ms);
        if(r==-1){
            std::cerr << RED_COLOR << "[CONEXION CERRADA]" << RESET_COLOR << '\n';
            game_->Quit();
            break;
        }
        //Mostrar en pantalla el mensaje de la forma "nick: mensaje"
        // std::cout << ms.nick << ": " << ms.message << '\n';

        switch (ms.type)
        {
            case Message::MESSAGE:{
                std::cout << CYAN_COLOR << "(chat) [" << ms.nick << "]: " << ms.message << RESET_COLOR << '\n';
                break;
            }
            //============================================================================================================================================
            // LOBBY MSG
            //============================================================================================================================================
            case Message::LOBBY_ACCEPT:{

                std::cout << MAGENTA_COLOR << "[LOBBY " << ms.lobbyName << " CREADO]:\nEsperando jugador..." << RESET_COLOR << '\n';
                lobbyName = ms.lobbyName;
                break;
            }
            case Message::LOBBY_DENY:{
                
                std::cout << RED_COLOR << "[LOBBY " << ms.lobbyName << " DENEGADO]:\nNombre repetido o máximo de lobbies creados." << RESET_COLOR << '\n';
                break;
            }
            case Message::LOBBY_SEND_LIST:{
                std::cout << MAGENTA_COLOR << "[LISTA DE LOBBIES]:" << RESET_COLOR << '\n';
                for(std::string lobbyName : ms.lobbyList){
                    if(lobbyName.find("none") == std::string::npos) std::cout << GREEN_COLOR << " " + lobbyName << RESET_COLOR << '\n';
                }
                break;
            }
            case Message::LOBBY_JOIN_DENY:{
                std::cout << RED_COLOR << "[LOBBY " << ms.lobbyName << " DENEGADO]:\nNo existe en la lista de lobbies." << RESET_COLOR << '\n';
                break;
            }
            case Message::LOBBY_QUIT_REPLY:{
                std::cout << YELLOW_COLOR << "[TU OPONENTE " << ms.lobbyName << " HA ABANDONADO EL LOBBY]" << RESET_COLOR << '\n';
                std::cout << MAGENTA_COLOR << "Volviendo al menu principal..." << RESET_COLOR << '\n' ;
                lobbyName = "";
                game_->endGame();
                break;
            }
        //============================================================================================================================================
        // PLAYERS MSG
        //============================================================================================================================================
            case Message::INITIAL_TURN:{
                std::cout << MAGENTA_COLOR << "[EMPIEZA LA PARTIDA EN LOBBY " << ms.lobbyName << "]" << RESET_COLOR << '\n';
                lobbyName = ms.lobbyName;

                if (ms.playerTurn){
                    std::cout << GREEN_COLOR << "[TU TURNO]"<< RESET_COLOR << '\n';
                }
                else{
                    std::cout << GREEN_COLOR << "[TURNO DEL OPONENTE]" << RESET_COLOR << '\n';;
                }
                game_->startGame(ms.playerTurn);
                break;
            }
            case Message::PLAYER_PLAY:{
                game_->reproducePlay(ms.posX,ms.posY);
                
                if (ms.playerWon) game_->gameFinished(!ms.playerTurn);
                else {
                    game_->setTurn(ms.playerTurn);
                    if (ms.playerTurn) std::cout << GREEN_COLOR << "[TU TURNO]"<< RESET_COLOR << '\n';
                    else std::cout << GREEN_COLOR << "[TURNO DEL OPONENTE]" << RESET_COLOR << '\n';;
                }
                break;
            }
            default:
                std::cerr << "Tipo de mensaje no soportado.\n";
			break;
        }
    }
}
/**
 * Manda la jugada de un cliente al servidor
 * param x: Posición x de la ficha colocada
 * param y: Posición y de la ficha colocada
 * param winningPlay: Indica si la jugada del jugador ha conseguido su victoria. (Fin de la partida)
 */
void Client::sendPlay(int x, int y, bool winningPlay){
    Message em(nick, "play", lobbyName);
    em.type = Message::PLAYER_PLAY;

    em.posX = x;
    em.posY = y;
    em.playerTurn = true;
    em.playerWon = winningPlay;

    socket.send(em);
}