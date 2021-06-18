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

            //Recibir Mensajes en y en función del tipo de mensaje
            // - LOGIN: Añadir al vector clients
            // - LOGOUT: Eliminar del vector clients
            // - MESSAGE: Reenviar el mensaje a todos los clientes (menos el emisor)

            Message cm;

            int soc = clientSocket_->recv(cm);
            if(soc==-1){
                std::cerr << "Error en socket.recv()\n";
                return;
            }

            switch (cm.type)
            {
            case Message::LOGIN:{
                std::unique_ptr<Socket> uS(clientSocket_);
                c_mtx->lock();
                clientsVector->push_back(std::move(uS));
                c_mtx->unlock();
                std::cout << "[ " << cm.nick << " joined the chat ]\n";
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
                    std::cout << "[ " << cm.nick << " left the chat ]\n";
                    active = false;
                }
                break;
            }
            case Message::MESSAGE:{
                for(auto &cs : *clientsVector){
                    if(*(cs.get()) == *clientSocket_) continue;
                    else{
                        int s = (cs.get())->send(cm);
                        if(s==-1){
                            std::cerr << "[message]: No se ha enviado el mensage correctamente\n";
                            return;
                        }
                    }
                }
                break;
            }
            case Message::LOBBY:{
                LobbyMessage lm;

                int soc = clientSocket_->recv(lm);
                if(soc==-1){
                    std::cerr << "Error en socket.recv()\n";
                }

                switch (lm.type)
                {
                    case LobbyMessage::LOBBY_REQUEST:{
                        // Logica para comprobar si se puede crear un lobby
                        // con lm.lobbyName nombre y crearlo, de momento se acepta sin mas

                        //Especificar primero el tipo de mensaje que se quiere tratar
                        Message em("server","");
                        em.type = Message::LOBBY;

                        clientSocket_->send(em);

                        //Enviar el mensaje del tipo correspondiente
                        sleep(SYNC_DELAY);
                        l_mtx->lock();
                        if(lobbiesMap->count(lm.lobbyName) > 0 || lobbiesMap->size()>=10){
                            lm.type = LobbyMessage::LOBBY_DENY;
                            clientSocket_->send(lm);
                        }
                        else{
                            lobbiesMap->insert({lm.lobbyName, std::make_pair(clientSocket_,nullptr)});
                            lm.type = LobbyMessage::LOBBY_ACCEPT;
                            clientSocket_->send(lm);
                        }
                        l_mtx->unlock();

                        break;
                    }
                    case LobbyMessage::LOBBY_ASK_LIST:{
                        // Logica para enviar la lista de lobbies disponibles
                        // al cliente

                        //Especificar primero el tipo de mensaje que se quiere tratar
                        Message em("server","");
                        em.type = Message::LOBBY;

                        clientSocket_->send(em);

                        //Enviar el mensaje del tipo correspondiente
                        sleep(SYNC_DELAY);
                        lm.type = LobbyMessage::LOBBY_SEND_LIST;
                        lm.empty_list();

                        int i = 0;
                        l_mtx->lock();
                        for (auto const& lobby : *lobbiesMap)
                        {
                            if(lobby.second.second == nullptr){
                                lm.lobbyList[i] = lobby.first;
                                i++;
                            }
                        }
                        l_mtx->unlock();

                        clientSocket_->send(lm);
                        
                        break;
                    }
                    case LobbyMessage::LOBBY_JOIN_REQUEST:{
                        Message em("server","");
                        em.type = Message::LOBBY;

                        clientSocket_->send(em);

                        //Enviar el mensaje del tipo correspondiente
                        sleep(SYNC_DELAY);
                        l_mtx->lock();
                        auto lobbyIt = lobbiesMap->find(lm.lobbyName);
                        std::pair<Socket*, Socket*>* lobbyPair = &lobbyIt->second;
                        if(lobbiesMap->count(lm.lobbyName) == 0 ||
                        lobbiesMap->count(lm.lobbyName)>0 && lobbyPair->second != nullptr){
                            lm.type = LobbyMessage::LOBBY_JOIN_DENY;
                            clientSocket_->send(lm);
                        }
                        else{
                            //Añadir el otro socket al mapa.
                            lobbyPair->second = clientSocket_;
                            lm.type = LobbyMessage::LOBBY_JOIN_ACCEPT;
                            clientSocket_->send(lm);

                            lobbyPair->first->send(em);
                            sleep(SYNC_DELAY);
                            lobbyPair->first->send(lm);

                            startGame(lobbyPair->first, lobbyPair->second);
                        }
                        l_mtx->unlock();

                        break;
                    }
                    case LobbyMessage::LOBBY_QUIT:{
                        auto mapElem = lobbiesMap->find(lm.lobbyName); //Encontramos al oponente
                        std::pair<Socket*, Socket*>* sockPair = &mapElem->second;
                        Socket* opponentSocket = nullptr;
                        if (sockPair->first == clientSocket_) {
                            opponentSocket = sockPair->second;
                        }
                        else opponentSocket = sockPair->first;

                        Message em("server","");
                        em.type = Message::LOBBY;

                        if (opponentSocket){
                            opponentSocket->send(em);
                            sleep(SYNC_DELAY);

                            lm.type = LobbyMessage::LOBBY_QUIT_REPLY; 
                            opponentSocket->send(lm); 
                        } 

                        lobbiesMap->erase(mapElem); //Acaba la partida y los eliminamos del mapa.
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

                int soc = clientSocket_->recv(pm);
                if(soc==-1){
                    std::cerr << "Error en socket.recv()\n";
                }

                switch (pm.type){
                    case PlayMessage::PLAYER_PLAY:{
                        auto mapElem = lobbiesMap->find(pm.lobbyName);
                        std::pair<Socket*, Socket*>* sockPair = &mapElem->second;

                        Message em("server","");
                        em.type = Message::PLAY;
                        sockPair->first->send(em);
                        sockPair->second->send(em);
                        sleep(SYNC_DELAY);

						PlayMessage pmSend;
						pmSend.type = PlayMessage::PLAYER_PLAY;
						pmSend.posX = pm.posX;
						pmSend.posY = pm.posY;

                        if (sockPair->first == clientSocket_){
                            pmSend.playerTurn = false;
							sockPair->first->send(pmSend);
							pmSend.playerTurn = true;
							sockPair->second->send(pmSend);
                        }
						else{
							pmSend.playerTurn = true;
							sockPair->first->send(pmSend);
							pmSend.playerTurn = false;
							sockPair->second->send(pmSend);
						}
						break;
                    }
					default:
						std::cout << "Tipo de mensaje no soportado. Esperaba uno de tipo PLAY. Esto es server.\n";
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
    
private:

void startGame(Socket* player1, Socket* player2){
    Socket* firstPlayer;
    Socket* secondPlayer;

    int random = rand() % 2;
    if (random == 0){
		std::cout << "Empieza el host\n";
        firstPlayer = player1;
        secondPlayer = player2;
    }
    else{
		std::cout << "Empieza el visitante\n";
        firstPlayer = player2;
        secondPlayer = player1;
    }

    Message em("server","");
    em.type = Message::PLAY;

    firstPlayer->send(em);
    secondPlayer->send(em);
    sleep(SYNC_DELAY);

    PlayMessage pm("");
    pm.type = PlayMessage::MessageType::INITIAL_TURN;
    pm.playerTurn = true;
    firstPlayer->send(pm);

    pm.playerTurn = false;
    secondPlayer->send(pm);
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