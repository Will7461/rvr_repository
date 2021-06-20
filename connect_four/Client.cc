#include "Client.h"
#include "SDLGame.h"
#include <time.h>
#include <stdlib.h>

Client::Client(const char * s, const char * p, const char * n, SDLGame* g) : socket(s, p),
nick(n), game_(g){
    socket.connect();
    game_->setClient(this);
}

void Client::login()
{
    std::string msg;

    Message em(nick, msg, "");
    em.type = Message::LOGIN;

    socket.send(em);
}

void Client::logout()
{
    // Completar
    std::string msg;

    Message em(nick, msg, "");
    em.type = Message::LOGOUT;

    socket.send(em);
}

void Client::leaveLobby(){
    std::cout << YELLOW_COLOR << "Abandonando la partida..." << RESET_COLOR << '\n';

    Message em(nick,"",lobbyName);
    em.type = Message::LOBBY_QUIT;
    socket.send(em);

    game_->endGame();
}

void Client::input_thread()
{
    while (true)
    {
        // Leer stdin con std::getline
        std::string msg;
        std::getline(std::cin, msg);
        if(msg=="!q" || msg=="quit"){
            if(game_->getPlaying()) leaveLobby();
            logout();
            game_->Quit();
            break;
        }
        else if(msg.length()>80){
            std::cerr << "Mensaje demasiado largo para ser enviado\n";
            continue;
        }

        if(msg == "create"){
            std::cout << "Introduce nombre de lobby: ";
            std::string lName;
            std::getline(std::cin, lName);

            Message em(nick,msg,lName);
            em.type = Message::LOBBY_REQUEST;

            socket.send(em);
        }
        else if(msg == "list"){

            Message em(nick,msg,"");
            em.type = Message::LOBBY_ASK_LIST;

            socket.send(em);
        }
        else if(msg == "join"){
            std::cout << "Introduce nombre de lobby al que quieres unirte: ";
            std::string lName;
            std::getline(std::cin, lName);

            Message em(nick,msg,lName);
            em.type = Message::LOBBY_JOIN_REQUEST;
            socket.send(em);
        }
        else if(msg == "abandon"){
            leaveLobby();
        }
        else if(game_->getPlaying()){
            Message em(nick,msg,lobbyName);
            em.type = Message::MESSAGE;
            socket.send(em);
        }
    }
}

void Client::net_thread()
{
    while(true)
    {
        //Recibir Mensajes de red
        Message ms;

        int r = socket.recv(ms);
        if(r==-1){
            std::cerr << CYAN_COLOR << "[CONEXION CERRADA]" << RESET_COLOR << '\n';
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
                //Habrá que guardar la lobby aquí supongo
                break;
            }
            case Message::LOBBY_QUIT_REPLY:{
                std::cout << YELLOW_COLOR << "[TU OPONENTE " << ms.lobbyName << " HA ABANDONADO LA PARTIDA]" << RESET_COLOR << '\n';
                std::cout << "Volviendo al menu principal...\n";
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
                std::cout << ms.playerWon << "\n";
                game_->reproducePlay(ms.posX,ms.posY);
                std::cout << "New ficha just dropped on: " << ms.posX << " " << ms.posY << "\n";
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

void Client::sendPlay(int x, int y, bool winningPlay){

    Message em(nick, "play", lobbyName);
    em.type = Message::PLAYER_PLAY;

    em.posX = x;
    em.posY = y;
    em.playerTurn = true;
    em.playerWon = winningPlay;

    socket.send(em);
}

