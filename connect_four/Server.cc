#include "Server.h"
#include <time.h>
#include <stdlib.h>

class MessageThread
{
public:
    MessageThread(int sd, struct sockaddr client, socklen_t clientlen,
    std::mutex* lobbies_mtx, std::map<std::string, std::pair<Socket*,Socket*>>* _lobbiesMap) : 
     l_mtx(lobbies_mtx), lobbiesMap(_lobbiesMap) {

        clientSocket_ = std::make_unique<Socket>(sd, &client, clientlen);

    };
    ~MessageThread(){
        
    };
    
    void do_conexion()
    {
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

            int soc = clientSocket_->recv(cm);
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
                std::cout << GREEN_COLOR << "[" << cm.nick << " JOINED THE SERVER]" << RESET_COLOR << '\n';
                break;
            }
            case Message::LOGOUT:{  
                std::cout << YELLOW_COLOR << "[" << cm.nick << " LEFT THE SERVER]" << RESET_COLOR << '\n';
                active = false;                  
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
                    clientSocket_->send(em);
                }

                //Añade la lobby al mapa y guarda el cliente como host de la sala.
                else{
                    lobbiesMap->insert({cm.lobbyName, std::make_pair(clientSocket_.get(),nullptr)});
                    em.type = Message::LOBBY_ACCEPT;
                    clientSocket_->send(em);
                }
                l_mtx->unlock();

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

                clientSocket_->send(em);
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
                    clientSocket_->send(em);
                }

                //Añadimos el cliente al lobby(dentro del mapa) y empieza la partida.
                else{
                    //Añadir el otro socket al mapa.
                    lobbyPair->second = clientSocket_.get();

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

                //Distinguimos entre host/visitante porque el visitante le comunicaremos
                //que el oponente ha dejado la sala. 
                if (sockPair->first == clientSocket_.get()) {
                    opponentSocket = sockPair->second;
                }
                else opponentSocket = sockPair->first;

                Message em("server","",cm.lobbyName);

                if (opponentSocket){
                    em.type = Message::LOBBY_QUIT_REPLY; 
                    opponentSocket->send(em); 
                }

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
                if (sockPair->first == clientSocket_.get()){
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
		std::cout << BLUE_COLOR << "[TURNO INICIAL PARA ANFITRION]" << RESET_COLOR << '\n';
        firstPlayer = player1;
        secondPlayer = player2;
    }
    else{
		std::cout << BLUE_COLOR << "[TURNO INICIAL PARA VISITANTE]" << RESET_COLOR << '\n';
        firstPlayer = player2;
        secondPlayer = player1;
    }

    m.type = Message::INITIAL_TURN;
    m.playerTurn = true;
    firstPlayer->send(m);

    m.playerTurn = false;
    secondPlayer->send(m);
}

std::unique_ptr<Socket> clientSocket_;

std::mutex* l_mtx;
std::map<std::string, std::pair<Socket*,Socket*>>* lobbiesMap;

};

// -----------------------------------------------------------------------------
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
            return;
        }

        MessageThread *mt = new MessageThread(client_sd, client, clientlen, &lobbies_mtx, &lobbies);
        printf("Address of new client is %p\n", (void *)mt);
        //Crea un hilo para enviar/recibir mensajes de ese cliente en concrecto.
        std::thread([&mt](){
            mt->do_conexion();

            delete mt;
        }).detach();
    }
}