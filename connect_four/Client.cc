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

    Message em(nick, msg);
    em.type = Message::LOGIN;

    socket.send(em);
}

void Client::logout()
{
    // Completar
    std::string msg;

    Message em(nick, msg);
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
            //Especificar primero el tipo de mensaje que se quiere tratar
            Message em(nick,msg);
            em.type = Message::LOBBY;

            socket.send(em);

            //Enviar el mensaje del tipo correspondiente
            sleep(SYNC_DELAY);
            std::cout << "Introduce nombre de lobby: ";
            std::string lName;
            std::getline(std::cin, lName);

            LobbyMessage lm(lName);
            lm.type = LobbyMessage::LOBBY_REQUEST;
            socket.send(lm);
        }
        else if(msg == "list"){
            Message em(nick,msg);
            em.type = Message::LOBBY;

            socket.send(em);

            sleep(SYNC_DELAY);
            LobbyMessage lm(msg);
            lm.type = LobbyMessage::LOBBY_ASK_LIST;
            socket.send(lm);
        }
        else if(msg == "join"){
            Message em(nick, msg);
            em.type = Message::LOBBY;

            socket.send(em);

            //Enviar el mensaje del tipo correspondiente
            sleep(SYNC_DELAY);
            std::cout << "Introduce nombre de lobby al que quieres unirte: ";
            std::string lName;
            std::getline(std::cin, lName);

            LobbyMessage lm(lName);
            lm.type = LobbyMessage::LOBBY_JOIN_REQUEST;
            socket.send(lm);
        }
        else if(msg == "abandon"){
            std::cout << "Abandonando la partida...\n";

            Message em(nick, msg);
            em.type = Message::LOBBY;

            socket.send(em);

            //Enviar el mensaje del tipo correspondiente
            sleep(SYNC_DELAY);
            LobbyMessage lm(lobbyName);
            lm.type = LobbyMessage::LOBBY_QUIT;
            socket.send(lm);

            game_->setPlaying(false);
            game_->resetTableRequest();
        }
        else if (msg == "play"){
            if (game_->getTurn()){
                Message em(nick, msg);
                em.type = Message::PLAY;

                socket.send(em);

                sleep(SYNC_DELAY);
                PlayMessage pm(lobbyName);
                pm.type = PlayMessage::PLAYER_PLAY;
                
                int posX, posY;
                std::cout << "PosX: ";
                std::cin >> posX;
                std::cout << "PosY: ";
                std::cin >> posY;

                pm.posX = posX;
                pm.posY = posY;
                pm.playerTurn = true;

                socket.send(pm);
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
            case Message::LOBBY:{
                LobbyMessage lm;

                int soc = socket.recv(lm);
                if(soc==-1){
                    std::cerr << "Error en socket.recv()\n";
                }

                switch (lm.type)
                {
                    case LobbyMessage::LOBBY_ACCEPT:{

                        std::cout << "Lobby " << lm.lobbyName << " creado\nEsperando jugador...\n";
                        lobbyName = lm.lobbyName;
                        break;
                    }
                    case LobbyMessage::LOBBY_DENY:{
                        
                        std::cout << "Lobby " << lm.lobbyName << " denegado por nombre repetido o máximo de lobbies creados.\n";
                        break;
                    }
                    case LobbyMessage::LOBBY_SEND_LIST:{
                        std::cout << "Lista de lobbies:\n";
                        for(std::string lobbyName : lm.lobbyList){
                            if(lobbyName.find("none") == std::string::npos) std::cout << lobbyName << '\n';
                        }
                        break;
                    }
                    case LobbyMessage::LOBBY_JOIN_ACCEPT:{
                        std::cout << "Empieza la partida en el lobby " << lm.lobbyName << "!\n";
                        lobbyName = lm.lobbyName;
                        break;
                    }
                    case LobbyMessage::LOBBY_JOIN_DENY:{
                        std::cout << "Lobby " << lm.lobbyName << " denegado por nombre incorrecto o porque no existe.\n";
                        //Habrá que guardar la lobby aquí supongo
                        break;
                    }
                    case LobbyMessage::LOBBY_QUIT_REPLY:{
                        std::cout << "Tu oponente " << lm.lobbyName << " ha abandonado la partida. Volviendo al menu principal.\n";
                        game_->setPlaying(false);
                        game_->resetTableRequest();
                        break;
                    }
                    default:
                        std::cout << "Tipo de mensaje no soportado. Esperaba uno de tipo LOBBY\n";
                        break;
                }

                break;
            }

            case Message::PLAY:{
                PlayMessage pm;
                int soc = socket.recv(pm);
                if(soc==-1){
                    std::cerr << "Error en socket.recv()\n";
                }
                switch(pm.type){
                    case PlayMessage::INITIAL_TURN:{
                        if (pm.playerTurn){
                            std::cout << "Partida empezada. Es mi turno.\n";
                        }
                        else{
                            std::cout << "Partida empezada. Es el turno del oponente\n";
                        }
                        game_->setPlaying(true);
                        game_->setTurn(pm.playerTurn);
                        if(game_->getTurn()) game_->setColor(Color::RED);
                        else game_->setColor(Color::YELLOW);
                        break;
                    }
					case PlayMessage::PLAYER_PLAY:{
                        game_->reproducePlay(pm.posX,pm.posY);
						std::cout << "New ficha just dropped on: " << pm.posX << " " << pm.posY << "\n";
						game_->setTurn(pm.playerTurn);
						if (pm.playerTurn) std::cout << "Es mi turno\n";
						else std::cout << "Es el turno del oponente\n";
						break;
					}

                    default:
                        std::cout << "Tipo de mensaje no soportado. Esperaba uno de tipo PLAY\n";
                    break;
                }
                break;
            }

            default:
                std::cerr << "Tipo de mensaje no soportado.\n";
			break;
        }
    }
}

void Client::sendPlay(int x, int y){

    Message em(nick, "play");
    em.type = Message::PLAY;

    socket.send(em);

    sleep(SYNC_DELAY);
    PlayMessage pm(lobbyName);
    pm.type = PlayMessage::PLAYER_PLAY;

    pm.posX = x;
    pm.posY = y;
    pm.playerTurn = true;

    socket.send(pm);
}

