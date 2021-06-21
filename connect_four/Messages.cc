#include "Messages.h"
#include <stdlib.h>

// -----------------------------------------------------------------------------

void Message::to_bin()
{
    alloc_data(MESSAGE_SIZE);

    memset(_data, 0, MESSAGE_SIZE);

    //Serializar los campos
    char* tmp = _data;

    memcpy(tmp, &type, sizeof(uint8_t));

    tmp += sizeof(uint8_t);

    memcpy(tmp, nick.c_str(), nick.size() + 1);

    tmp += 8 * sizeof(char);

    memcpy(tmp, message.c_str(), message.size() + 1);

    tmp += 80 * sizeof(char);

    memcpy(tmp, lobbyName.c_str(), lobbyName.size() + 1);

    for(int i = 0; i<MAX_LOBBIES; i++){
        tmp += sizeof(std::string);
        memcpy(tmp, lobbyList[i].c_str(), lobbyList[i].size() + 1);
    }

    tmp += sizeof(std::string);

    memcpy(tmp, &playerTurn, sizeof(bool));

    tmp += sizeof(bool);

    memcpy(tmp, &playerWon, sizeof(bool));

    tmp += sizeof(bool);

    memcpy(tmp, &posX, sizeof(int));

    tmp += sizeof(int);

    memcpy(tmp, &posY, sizeof(int));
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

    tmp += 80 * sizeof(char);

    lobbyName.resize(sizeof(std::string));
    memcpy((void *)lobbyName.c_str(), tmp, sizeof(std::string));

    for(int i = 0; i<MAX_LOBBIES; i++){
        tmp += sizeof(std::string);

        lobbyList[i].resize(sizeof(std::string));
        memcpy((void *)lobbyList[i].c_str(), tmp, sizeof(std::string));
    }

    tmp += sizeof(std::string);

    memcpy(&playerTurn, tmp, sizeof(bool));

    tmp += sizeof(bool);

    memcpy(&playerWon, tmp, sizeof(bool));

    tmp += sizeof(bool);

    memcpy(&posX, tmp, sizeof(int));

    tmp += sizeof(int);    

    memcpy(&posY, tmp, sizeof(int));

    return 0;
}