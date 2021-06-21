#include "Server.h"
#include <time.h>
#include <stdlib.h>
#include <signal.h>
#include <vector>

class MessageThread
{
public:
    MessageThread(int sd_, struct sockaddr client_, socklen_t clientlen_,
    std::mutex* lobbies_mtx, std::map<std::string, std::pair<Socket*,Socket*>>* lobbiesMap_) : sd(sd_), client(client_), clientlen(clientlen_),
     l_mtx(lobbies_mtx), lobbiesMap(lobbiesMap_){

    };
    
    void do_conexion()
    {
        Socket clientSocket_(sd, &client, clientlen);

        bool active = true;
        //Gestion de la conexion
        while (active)
        {
            /*
            * NOTA: los clientes están definidos con "smart pointers", es necesario
            * crear un unique_ptr con el objeto socket recibido y usar std::move
            * para añadirlo al vector
            */

            Message cm;

            int soc = clientSocket_.recv(cm);
            if(soc==-1){
                std::cerr << "Error en socket.recv()\n";
                return;
            }

            switch (cm.type)
            {
            //============================================================================================================================================
            // BASIC MSG
            //============================================================================================================================================
            
            //Registra al cliente en el vector de clientes.
            case Message::LOGIN:{
                std::cout << GREEN_COLOR << "[" << cm.nick << " SE UNE AL SERVIDOR]" << RESET_COLOR << '\n';
                break;
            }
            //Mensaje de chat
            case Message::MESSAGE:{
                auto mapElem = lobbiesMap->find(cm.lobbyName);
                std::pair<Socket*, Socket*>* sockPair = &mapElem->second;

                Message em(cm.nick,cm.message,cm.lobbyName);
                em.type = Message::MESSAGE;

                if(sockPair->first)
                sockPair->first->send(em);
                if(sockPair->second)
                sockPair->second->send(em);

                break;
            }
            case Message::LOGOUT:{  
                std::cout << RED_COLOR << "[" << cm.nick << " DEJA EL SERVIDOR]" << RESET_COLOR << '\n';
                active = false;                  
                break;
            }
            //============================================================================================================================================
            // LOBBY MSG
            //============================================================================================================================================
            
            //Petición de un cliente para crear una lobby.
            case Message::LOBBY_REQUEST:{
                Message em("server","",cm.lobbyName);
                
                l_mtx->lock();

                //Si la lobby indicada ya existe o hemos llegado al tope, rechazamos su petición.
                if(lobbiesMap->count(cm.lobbyName) > 0 || lobbiesMap->size()>=10){
                    em.type = Message::LOBBY_DENY;
                }
                //Añade la lobby al mapa y guarda el cliente como host de la sala.
                else{
                    lobbiesMap->insert({cm.lobbyName, std::make_pair(&clientSocket_,nullptr)});
                    em.type = Message::LOBBY_ACCEPT;
                }
                l_mtx->unlock();
                
                clientSocket_.send(em);

                std::cout << CYAN_COLOR << "[LOBBY "<< em.lobbyName << " CREADO]" << RESET_COLOR << '\n';

                break;
            }

            //Petición para devolver una lista con las salas disponibles. (No llenas)
            case Message::LOBBY_ASK_LIST:{
                Message em("server","","");
                em.type = Message::LOBBY_SEND_LIST;

                em.empty_list();

                int i = 0;
                l_mtx->lock();
                for (auto const& lobby : *lobbiesMap)
                {
                    if(lobby.second.second == nullptr){
                        em.lobbyList[i] = lobby.first;
                        i++;
                    }
                }
                l_mtx->unlock();

                clientSocket_.send(em);
                break;
            }

            //Petición de un cliente para entrar en una sala.
            case Message::LOBBY_JOIN_REQUEST:{
                Message em("server","", cm.lobbyName);

                l_mtx->lock();
                auto lobbyIt = lobbiesMap->find(cm.lobbyName);
                std::pair<Socket*, Socket*>* lobbyPair = &lobbyIt->second;

                //Si la lobby no existe o ya está llena, rechazamos su petición.
                if(lobbiesMap->count(cm.lobbyName) == 0 ||
                    lobbiesMap->count(cm.lobbyName)>0 && lobbyPair->second != nullptr){
                    em.type = Message::LOBBY_JOIN_DENY;
                    clientSocket_.send(em);
                }

                //Añadimos el cliente al lobby(dentro del mapa) y empieza la partida.
                else{
                    //Añadir el otro socket al mapa.
                    lobbyPair->second = &clientSocket_;

                    startGame(lobbyPair->first, lobbyPair->second, em);
                }
                l_mtx->unlock();

                break;  
            }

            //Uno de los clientes abandona la sala.
            case Message::LOBBY_QUIT:{
                auto mapElem = lobbiesMap->find(cm.lobbyName); //Encontramos la sala
                std::pair<Socket*, Socket*>* sockPair = &mapElem->second;
                Socket* opponentSocket = nullptr;

                //Avisamos al oponente que ha dejado la sala. 
                if (sockPair->first == &clientSocket_) {
                    opponentSocket = sockPair->second;
                }
                else opponentSocket = sockPair->first;

                Message em(cm.nick,"",cm.lobbyName);

                if (opponentSocket){
                    em.type = Message::LOBBY_QUIT_REPLY; 
                    opponentSocket->send(em);
                }

                std::cout << YELLOW_COLOR << "[" << cm.nick << " DEJA EL LOBBY " << cm.lobbyName << "]\nSe ha eliminado el lobby " << GREEN_COLOR << cm.lobbyName << YELLOW_COLOR << " de la lista." << RESET_COLOR << '\n';

                lobbiesMap->erase(mapElem); //Acaba la partida y los eliminamos del mapa.
                break;
            }
            //============================================================================================================================================
            // PLAYERS MSG
            //============================================================================================================================================
            
            //El cliente envía su jugada
            case Message::PLAYER_PLAY:{
                //Encontramos el lobby al que pertenece.
                auto mapElem = lobbiesMap->find(cm.lobbyName);
                std::pair<Socket*, Socket*>* sockPair = &mapElem->second;

                Message em("server","",cm.lobbyName);
                em.type = Message::PLAYER_PLAY;

                em.posX = cm.posX;
                em.posY = cm.posY;
                em.playerWon = cm.playerWon;
                //El siguiente turno será del jugador que NO nos haya enviado la jugada.
                if (sockPair->first == &clientSocket_){
                    em.playerTurn = false;
                    sockPair->first->send(em);
                    em.playerTurn = true;
                    sockPair->second->send(em);
                }
                else{
                    em.playerTurn = true;
                    sockPair->first->send(em);
                    em.playerTurn = false;
                    sockPair->second->send(em);
                }
                break;
            }
            default:
                std::cerr << "Tipo de mensaje no soportado.\n";
                break;
            }
        }
    }
    
private:
/**
 * Decide aleatoriamente que jugador dentro de la sala empieza la partida y se lo comunica.
 */
void startGame(Socket* player1, Socket* player2, Message m){
    Socket* firstPlayer;
    Socket* secondPlayer;

    int random = rand() % 2;
    if (random == 0){
		std::cout << BLUE_COLOR << "[TURNO INICIAL PARA ANFITRION EN LOBBY " << m.lobbyName << "]" << RESET_COLOR << '\n';
        firstPlayer = player1;
        secondPlayer = player2;
    }
    else{
		std::cout << BLUE_COLOR << "[TURNO INICIAL PARA VISITANTE EN LOBBY " << m.lobbyName << "]" << RESET_COLOR << '\n';
        firstPlayer = player2;
        secondPlayer = player1;
    }

    m.type = Message::INITIAL_TURN;
    m.playerTurn = true;
    firstPlayer->send(m);

    m.playerTurn = false;
    secondPlayer->send(m);
}

int sd;
struct sockaddr client;
socklen_t clientlen;

std::mutex* l_mtx;
std::map<std::string, std::pair<Socket*,Socket*>>* lobbiesMap;

};

// -----------------------------------------------------------------------------
void constructMessageThread(int sd_, struct sockaddr client_, socklen_t clientlen_,
    std::mutex* lobbies_mtx, std::map<std::string, std::pair<Socket*,Socket*>>* lobbiesMap_) {
    MessageThread mt(sd_, client_, clientlen_, lobbies_mtx, lobbiesMap_);
    mt.do_conexion();
}

void Server::input_thread(){
    while (true)
    {
        std::string inp;
        std::getline(std::cin, inp);
        if(inp =="Quit" || inp=="quit"){
            std::cerr << RED_COLOR << "[CERRANDO SERVIDOR]" << RESET_COLOR << '\n';
            socket.socketClose();
            break;
        }
    }
}

// -----------------------------------------------------------------------------
/**
 * Hilo para recibir conexiones de nuevos clientes.
 */
void Server::do_conexions()
{
    //Gestion de conexiones entrantes
    while (true)
    {
        struct sockaddr client;
        socklen_t clientlen = sizeof(struct sockaddr);
        int client_sd = socket.accept(client, clientlen);

        if( client_sd==-1 ){
            std::cerr << "Error at socket.accept()" << strerror(errno) << '\n';
            break;
        }

        std::thread thr(constructMessageThread,client_sd, client, clientlen, &lobbies_mtx, &lobbies);
        thr.detach();
    }
}