#include "Chat.h"

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

void ChatMessage::to_bin()
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

int ChatMessage::from_bin(char * bobj)
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

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

class MessageThread
{
public:
    MessageThread(int sd, struct sockaddr client, socklen_t clientlen, std::mutex& _mtx, std::vector<std::unique_ptr<Socket>>& _clientsVector) : clientSocket_(sd, &client, clientlen),
    mtx(_mtx), clientsVector(_clientsVector){};
    
    void do_conexion()
    {
        //Gestion de la conexion
        while (true)
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

            ChatMessage cm;
            Socket* s;
            int soc = clientSocket_.recv(cm);
            if(soc==-1){
                std::cerr << "Error en socket.recv()\n";
                return;
            }

            switch (cm.type)
            {
            case ChatMessage::LOGIN:{
                std::unique_ptr<Socket> uS(s);
                mtx.lock();
                clientsVector.push_back(std::move(uS));
                mtx.unlock();
                std::cout << "[ " << cm.nick << " joined the chat ]\n";
                break;
            }
            case ChatMessage::LOGOUT:{
                auto it = clientsVector.begin();
                while (it != clientsVector.end())
                {
                    if( *((*it).get()) == *s ) break;
                    ++it;
                }

                if(it == clientsVector.end()) std::cerr << "[logout] No se ha encontrado cliente que desconectar\n";
                else{
                    mtx.lock();
                    clientsVector.erase(it);
                    mtx.unlock();
                    std::cout << "[ " << cm.nick << " left the chat ]\n";
                }
                break;
            }
            case ChatMessage::MESSAGE:{
                for(auto &cs : clientsVector){
                    if(*(cs.get()) == *s) continue;
                    else{
                        int s = clientSocket_.send(cm, *(cs.get()));
                        if(s==-1){
                            std::cerr << "[message]: No se ha enviado el mensage correctamente\n";
                            return;
                        }
                    }
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
Socket clientSocket_;
std::mutex& mtx;
std::vector<std::unique_ptr<Socket>>& clientsVector;
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

        MessageThread *mt = new MessageThread(client_sd, client, clientlen, clients_mtx, clients);

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

    ChatMessage em(nick, msg);
    em.type = ChatMessage::LOGIN;

    socket.send(em, socket);
}

void ChatClient::logout()
{
    // Completar
    std::string msg;

    ChatMessage em(nick, msg);
    em.type = ChatMessage::LOGOUT;

    socket.send(em, socket);
}

void ChatClient::input_thread()
{
    while (true)
    {
        // Leer stdin con std::getline
        std::string msg;
        std::getline(std::cin, msg);

        if(msg=="!q"){
            logout();
            break;
        }
        else if(msg.length()>80){
            std::cerr << "Mensaje demasiado largo para ser enviado\n";
            continue;
        }

        ChatMessage em(nick,msg);
        em.type = ChatMessage::MESSAGE;

        // Enviar al servidor usando socket
        socket.send(em, socket);
    }
}

void ChatClient::net_thread()
{
    while(true)
    {
        //Recibir Mensajes de red
        ChatMessage ms;
        Socket* s;
        int r = socket.recv(ms, s);
        if(r==-1){
            std::cerr << "Error: no se ha recibido un mensaje correctamente";
            return;
        }
        //Mostrar en pantalla el mensaje de la forma "nick: mensaje"
        std::cout << ms.nick << ": " << ms.message << '\n';
    }
}

