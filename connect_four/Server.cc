#include "Server.h"
#include <time.h>
#include <stdlib.h>

class MessageThread
{
public:
    MessageThread(int sd, struct sockaddr client, socklen_t clientlen, std::mutex* clients_mtx, std::vector<std::unique_ptr<Socket>>* _clientsVector,
    std::mutex* lobbies_mtx, std::map<std::string, std::pair<Socket*,Socket*>>* _lobbiesMap) : 
    c_mtx(clients_mtx), clientsVector(_clientsVector), l_mtx(lobbies_mtx), lobbiesMap(_lobbiesMap) {

        clientSocket_ = new Socket(sd, &client, clientlen);

    };
    ~MessageThread(){
        c_mtx = nullptr;
        clientsVector = nullptr;
        clientSocket_ = nullptr;
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
            case Message::LOGIN:{
                std::unique_ptr<Socket> uS(clientSocket_);
                c_mtx->lock();
                clientsVector->push_back(std::move(uS));
                c_mtx->unlock();
                std::cout << GREEN_COLOR << "[" << cm.nick << " JOINED THE SERVER]" << RESET_COLOR << '\n';
                break;
            }
            case Message::LOGOUT:{
                auto it = clientsVector->begin();
                while (it != clientsVector->end())
                {
                    if( *((*it).get()) == *clientSocket_ ) break;
                    ++it;
                }

                if(it == clientsVector->end()) std::cerr << "[logout] No se ha encontrado cliente que desconectar\n";
                else{
                    c_mtx->lock();
                    clientsVector->erase(it);
                    c_mtx->unlock();
                    std::cout << YELLOW_COLOR << "[" << cm.nick << " LEFT THE SERVER]" << RESET_COLOR << '\n';
                    active = false;
                }
                break;
            }
            case Message::MESSAGE:{
                auto mapElem = lobbiesMap->find(cm.lobbyName);
                std::pair<Socket*, Socket*>* sockPair = &mapElem->second;

                Message em(cm.nick,cm.message,cm.lobbyName);
                em.type = Message::MESSAGE;

                sockPair->first->send(em);
                sockPair->second->send(em);

                break;
            }
            //============================================================================================================================================
            // LOBBY MSG
            //============================================================================================================================================
            case Message::LOBBY_REQUEST:{
                // Logica para comprobar si se puede crear un lobby

                Message em("server","",cm.lobbyName);
                
                l_mtx->lock();
                if(lobbiesMap->count(cm.lobbyName) > 0 || lobbiesMap->size()>=10){
                    em.type = Message::LOBBY_DENY;
                    clientSocket_->send(cm);
                }
                else{
                    lobbiesMap->insert({cm.lobbyName, std::make_pair(clientSocket_,nullptr)});
                    em.type = Message::LOBBY_ACCEPT;
                    clientSocket_->send(em);
                }
                l_mtx->unlock();

                break;
            }
            case Message::LOBBY_ASK_LIST:{
                // Logica para enviar la lista de lobbies disponibles
                // al cliente

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
            case Message::LOBBY_JOIN_REQUEST:{
                Message em("server","", cm.lobbyName);

                l_mtx->lock();
                auto lobbyIt = lobbiesMap->find(cm.lobbyName);
                std::pair<Socket*, Socket*>* lobbyPair = &lobbyIt->second;
                if(lobbiesMap->count(cm.lobbyName) == 0 ||
                    lobbiesMap->count(cm.lobbyName)>0 && lobbyPair->second != nullptr){
                    em.type = Message::LOBBY_JOIN_DENY;
                    clientSocket_->send(em);
                }
                else{
                    //Añadir el otro socket al mapa.
                    lobbyPair->second = clientSocket_;

                    startGame(lobbyPair->first, lobbyPair->second, em);
                }
                l_mtx->unlock();

                break;  
            }
            case Message::LOBBY_QUIT:{
                auto mapElem = lobbiesMap->find(cm.lobbyName); //Encontramos al oponente
                std::pair<Socket*, Socket*>* sockPair = &mapElem->second;
                Socket* opponentSocket = nullptr;
                if (sockPair->first == clientSocket_) {
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
            case Message::PLAYER_PLAY:{
                auto mapElem = lobbiesMap->find(cm.lobbyName);
                std::pair<Socket*, Socket*>* sockPair = &mapElem->second;

                Message em("server","",cm.lobbyName);
                em.type = Message::PLAYER_PLAY;

                em.posX = cm.posX;
                em.posY = cm.posY;
                em.playerWon = cm.playerWon;
                if (sockPair->first == clientSocket_){
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

Socket* clientSocket_;
std::mutex* c_mtx;
std::vector<std::unique_ptr<Socket>>* clientsVector;

std::mutex* l_mtx;
std::map<std::string, std::pair<Socket*,Socket*>>* lobbiesMap;

};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

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

        MessageThread *mt = new MessageThread(client_sd, client, clientlen, &clients_mtx, &clients,
        &lobbies_mtx, &lobbies);

        std::thread([&mt](){
            mt->do_conexion();

            delete mt;
        }).detach();
    }
}