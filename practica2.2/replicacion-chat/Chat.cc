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

    if(nick.size()>7){
        std::cerr << "Nick demasiado largo para serializar\n"; 
        return;
    }

    nick[nick.size()] = '\0';

    memcpy(tmp, nick.c_str(), nick.size() + 1);

    tmp += 8 * sizeof(char);

    if(message.size()>79){
        std::cerr << "Message demasiado largo para serializar\n"; 
        return;
    }

    message[message.size()] = '\0';

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

    memcpy((void *)nick.c_str(), tmp, 8 * sizeof(char));

    tmp += 8 * sizeof(char);

    memcpy((void *)message.c_str(), tmp, 80 * sizeof(char));

    return 0;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

void ChatServer::do_messages()
{
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
}

void ChatClient::input_thread()
{
    while (true)
    {
        // Leer stdin con std::getline
        // Enviar al servidor usando socket
    }
}

void ChatClient::net_thread()
{
    while(true)
    {
        //Recibir Mensajes de red
        //Mostrar en pantalla el mensaje de la forma "nick: mensaje"
    }
}

