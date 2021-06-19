#pragma once
#include <string>
#include <unistd.h>
#include <thread>
#include <mutex>

#include "Socket.h"
#include "Messages.h"

/**
 *  Clase para el cliente
 */
class SDLGame;
class Client
{
public:
    /**
     * @param s dirección del servidor
     * @param p puerto del servidor
     * @param n nick del usuario
     */
    Client(const char * s, const char * p, const char * n, SDLGame* g);

    /**
     *  Envía el mensaje de login al servidor
     */
    void login();

    /**
     *  Envía el mensaje de logout al servidor
     */
    void logout();

    /**
     *  Rutina principal para el Thread de E/S. Lee datos de STDIN (std::getline)
     *  y los envía por red vía el Socket.
     */
    void input_thread();

    /**
     *  Rutina del thread de Red. Recibe datos de la red y los "renderiza"
     *  en STDOUT
     */
    void net_thread();

    void sendPlay(int x, int y, bool winningPlay = false);

private:

    /**
     * Socket para comunicar con el servidor
     */
    Socket socket;

    /**
     * Nick del usuario
     */
    std::string nick;

    /**
     * Lobby en la que está asignado el usuario
     */
    std::string lobbyName;

    SDLGame* game_;
};

