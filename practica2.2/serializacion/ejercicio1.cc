#include "Serializable.h"

#include <iostream>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

class Jugador: public Serializable
{
public:
    Jugador(const char * _n, int16_t _x, int16_t _y): pos_x(_x), pos_y(_y)
    {
        strncpy(name, _n, MAX_NAME);
    };

    virtual ~Jugador(){};

    void to_bin()
    {
        int32_t data_size = MAX_NAME * sizeof(char) + 2 * sizeof(int16_t);
        alloc_data(data_size);

        char* tmp = _data;

        memcpy(tmp, name, MAX_NAME * sizeof(char));

        tmp += MAX_NAME * sizeof(char);

        memcpy(tmp, &pos_x, sizeof(int16_t));

        tmp += sizeof(int16_t);

        memcpy(tmp, &pos_y, sizeof(int16_t));
    }

    int from_bin(char * data)
    {
        char* tmp = data;

        memcpy(name, tmp, MAX_NAME * sizeof(char));

        tmp += MAX_NAME * sizeof(char);

        memcpy(&pos_x, tmp, sizeof(int16_t));

        tmp += sizeof(int16_t);

        memcpy(&pos_y, tmp, sizeof(int16_t));

        return 0;
    }

    void show_info(){
        std::cout << "Nombre: " << name << " Pos X: " << pos_x << " Pos Y: " << pos_y << '\n';
    }


public:
  static const size_t MAX_NAME = 20;

  int16_t pos_x;
  int16_t pos_y;
  
  char name[MAX_NAME];
};

int main(int argc, char **argv)
{
    Jugador one_r("", 0, 0);
    srand (time(NULL));
    int randomX = rand() % 1001 + (-500);
    int randomY = rand() % 1001 + (-500);
    Jugador one_w("Player_ONE", randomX, randomY);

    // 1. Serializar el objeto one_w
    one_w.to_bin();
    // 2. Escribir la serialización en un fichero
    int fd = open("./data_jugador", O_CREAT | O_TRUNC | O_RDWR, 0666);

    write(fd, one_w.data(), one_w.size());

    close(fd);
    // 3. Leer el fichero
    fd = open("./data_jugador", O_RDONLY);
    
    int size = Jugador::MAX_NAME * sizeof(char) + 2 * sizeof(int16_t);
    char buff[size];
    read(fd, buff, size);

    close(fd);
    // 4. "Deserializar" en one_r
    one_r.from_bin(buff);
    // 5. Mostrar el contenido de one_r
    one_r.show_info();

    return 0;
}

