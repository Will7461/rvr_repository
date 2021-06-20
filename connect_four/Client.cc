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

void Client::input_thread()
{
    while (true)
    {
        // Leer stdin con std::getline
        std::string msg;
        std::getline(std::cin, msg);
        if(msg=="!q" || msg=="quit"){
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
            std::cout << "Abandonando la partida...\n";

            Message em(nick,msg,lobbyName);
            em.type = Message::LOBBY_QUIT;
            socket.send(em);

            game_->endGame();
        }
        else if (msg == "play"){
            if (game_->getTurn()){
                Message em(nick, msg, lobbyName);
                em.type = Message::PLAYER_PLAY;
                int posX, posY;
                std::cout << "PosX: ";
                std::cin >> posX;
                std::cout << "PosY: ";
                std::cin >> posY;

                em.posX = posX;
                em.posY = posY;
                em.playerTurn = true;

                socket.send(em);
            }
            else{
                std::cout << "No es tu turno. No puedes realizar una jugada\n";
            }
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
            std::cerr << "Conexion cerrada.\n";
            break;
        }
        //Mostrar en pantalla el mensaje de la forma "nick: mensaje"
        // std::cout << ms.nick << ": " << ms.message << '\n';

        switch (ms.type)
        {
            case Message::MESSAGE:{
                std::cout << ms.message << '\n';
                break;
            }
            //============================================================================================================================================
            // LOBBY MSG
            //============================================================================================================================================
            case Message::LOBBY_ACCEPT:{

                std::cout << "Lobby " << ms.lobbyName << " creado\nEsperando jugador...\n";
                lobbyName = ms.lobbyName;
                break;
            }
            case Message::LOBBY_DENY:{
                
                std::cout << "Lobby " << ms.lobbyName << " denegado por nombre repetido o máximo de lobbies creados.\n";
                break;
            }
            case Message::LOBBY_SEND_LIST:{
                std::cout << "Lista de lobbies:\n";
                for(std::string lobbyName : ms.lobbyList){
                    if(lobbyName.find("none") == std::string::npos) std::cout << lobbyName << '\n';
                }
                break;
            }
            case Message::LOBBY_JOIN_DENY:{
                std::cout << "Lobby " << ms.lobbyName << " denegado por nombre incorrecto o porque no existe.\n";
                //Habrá que guardar la lobby aquí supongo
                break;
            }
            case Message::LOBBY_QUIT_REPLY:{
                std::cout << "Tu oponente " << ms.lobbyName << " ha abandonado la partida. Volviendo al menu principal.\n";
                game_->endGame();
                break;
            }
        //============================================================================================================================================
        // PLAYERS MSG
        //============================================================================================================================================
            case Message::INITIAL_TURN:{
                std::cout << "Empieza la partida en el lobby " << ms.lobbyName << "!\n";
                lobbyName = ms.lobbyName;

                std::cout << "Partida empezada en lobby " << ms.lobbyName << ". ";
                if (ms.playerTurn){
                    std::cout << "Es mi turno.\n";
                }
                else{
                    std::cout << "Es el turno del oponente\n";
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
                    if (ms.playerTurn) std::cout << "Es mi turno\n";
                    else std::cout << "Es el turno del oponente\n";
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

