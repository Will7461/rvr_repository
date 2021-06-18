#include "Chat.h"
#include <time.h>
#include <stdlib.h>

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

void Message::to_bin()
{
    alloc_data(MESSAGE_SIZE);

    memset(_data, 0, MESSAGE_SIZE);

    //Serializar los campos type, nick y message en el buffer _data
    char* tmp = _data;

    memcpy(tmp, &type, sizeof(uint8_t));

    tmp += sizeof(uint8_t);

    memcpy(tmp, nick.c_str(), nick.size() + 1);

    tmp += 8 * sizeof(char);

    memcpy(tmp, message.c_str(), message.size() + 1);
}

int Message::from_bin(char * bobj)
{
    alloc_data(MESSAGE_SIZE);

    memcpy(static_cast<void *>(_data), bobj, MESSAGE_SIZE);

    //Reconstruir la clase usando el buffer _data

    if(_data==NULL) return -1;

    char* tmp = _data;

    memcpy(&type, tmp, sizeof(uint8_t));

    tmp += sizeof(uint8_t);

    nick.resize(8 * sizeof(char));
    memcpy((void*)nick.c_str(), tmp, 8 * sizeof(char));

    tmp += 8 * sizeof(char);

    message.resize(80 * sizeof(char));
    memcpy((void *)message.c_str(), tmp, 80 * sizeof(char));

    return 0;
}

void LobbyMessage::to_bin()
{
    alloc_data(MESSAGE_SIZE);

    memset(_data, 0, MESSAGE_SIZE);

    //Serializar los campos type, nick y message en el buffer _data
    char* tmp = _data;

    memcpy(tmp, &type, sizeof(uint8_t));

    tmp += sizeof(uint8_t);

    memcpy(tmp, lobbyName.c_str(), lobbyName.size() + 1);

    for(int i = 0; i<MAX_LOBBIES; i++){
        tmp += sizeof(std::string);
        memcpy(tmp, lobbyList[i].c_str(), lobbyList[i].size() + 1);
    }
}

int LobbyMessage::from_bin(char * bobj)
{
    alloc_data(MESSAGE_SIZE);

    memcpy(static_cast<void *>(_data), bobj, MESSAGE_SIZE);

    //Reconstruir la clase usando el buffer _data

    if(_data==NULL) return -1;

    char* tmp = _data;

    memcpy(&type, tmp, sizeof(uint8_t));

    tmp += sizeof(uint8_t);

    lobbyName.resize(sizeof(std::string));
    memcpy((void *)lobbyName.c_str(), tmp, sizeof(std::string));

    for(int i = 0; i<MAX_LOBBIES; i++){
        tmp += sizeof(std::string);

        lobbyList[i].resize(sizeof(std::string));
        memcpy((void *)lobbyList[i].c_str(), tmp, sizeof(std::string));
    }

    return 0;
}

void PlayMessage::to_bin(){

    alloc_data(MESSAGE_SIZE);

    memset(_data, 0, MESSAGE_SIZE);

    //Serializar los campos type, nick y message en el buffer _data
    char* tmp = _data;

    memcpy(tmp, &type, sizeof(uint8_t));

    tmp += sizeof(uint8_t);
    
    memcpy(tmp, lobbyName.c_str(), lobbyName.size() + 1);

    tmp += sizeof(std::string);

    memcpy(tmp, &playerTurn, sizeof(bool));

    tmp += sizeof(bool);

    memcpy(tmp, &posX, sizeof(int));

    tmp += sizeof(int);

    memcpy(tmp, &posY, sizeof(int));

    tmp += sizeof(int);
}

int PlayMessage::from_bin(char* bobj){

    alloc_data(MESSAGE_SIZE);

    memcpy(static_cast<void *>(_data), bobj, MESSAGE_SIZE);

    //Reconstruir la clase usando el buffer _data

    if(_data==NULL) return -1;

    char* tmp = _data;

    memcpy(&type, tmp, sizeof(uint8_t));

    tmp += sizeof(uint8_t);

    lobbyName.resize(sizeof(std::string));
    memcpy((void *)lobbyName.c_str(), tmp, sizeof(std::string));

    tmp += sizeof(std::string);

    memcpy(&playerTurn, tmp, sizeof(bool));

    tmp += sizeof(bool);

    memcpy(&posX, tmp, sizeof(int));

    tmp += sizeof(int);    

    memcpy(&posY, tmp, sizeof(int));

    tmp += sizeof(int);  

    return 0;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

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


void ChatServer::do_conexions()
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

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

void ChatClient::login()
{
    std::string msg;

    Message em(nick, msg);
    em.type = Message::LOGIN;

    socket.send(em);
}

void ChatClient::logout()
{
    // Completar
    std::string msg;

    Message em(nick, msg);
    em.type = Message::LOGOUT;

    socket.send(em);
}

void ChatClient::input_thread()
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
            std::string lobbyName;
            std::getline(std::cin, lobbyName);

            LobbyMessage lm(lobbyName);
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
            std::string lobbyName;
            std::getline(std::cin, lobbyName);

            LobbyMessage lm(lobbyName);
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
        }
        else if (msg == "play"){
            if (game_->getTurn()){
                Message em(nick, msg);
                em.type = Message::PLAY;

                socket.send(em);

                sleep(SYNC_DELAY);
                PlayMessage lp(lobbyName);
                lp.type = PlayMessage::PLAYER_PLAY;
                
                int posX, posY;
                std::cout << "PosX: ";
                std::cin >> posX;
                std::cout << "PosY: ";
                std::cin >> posY;

                lp.posX = posX;
                lp.posY = posY;
                lp.playerTurn = true;

                socket.send(lp);
            }
            else{
                std::cout << "No es tu turno. No puedes realizar una jugada\n";
            }
        }
        // ChatMessage em(nick,msg);
        // em.type = ChatMessage::MESSAGE;

        // // Enviar al servidor usando socket
        // socket.send(em);
    }
}

void ChatClient::net_thread()
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
                        game_->setTurn(pm.playerTurn);
                        break;
                    }
					case PlayMessage::PLAYER_PLAY:{
						std::cout << "New ficha just dropped on: " << pm.posX << " " << pm.posY << "\n";
						game_->setTurn(pm.playerTurn);
						if (pm.playerTurn) std::cout << "Es mi turno\n";
						else std::cout << "El el turno del oponente\n";
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

