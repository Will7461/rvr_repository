CC=g++
CFLAGS=-g -I.
DEPS = Socket.h Server.h Client.h Messages.h SDLGame.h Texture.h
OBJ = Socket.o Server.o Client.o Messages.o SDLGame.o Texture.o
LIBS=-lpthread -lSDL2 -lSDL2_ttf -lSDL2_image

%.o: %.cc $(DEPS)
	$(CC) -g -c -o $@ $< $(CFLAGS)

all: cs cc

cs: $(OBJ) ServerLauncher.o
	g++ -o $@ $^ $(CFLAGS) $(LIBS)

cc: $(OBJ) ClientLauncher.o
	g++ -o $@ $^ $(CFLAGS) $(LIBS)


.PHONY: clean

clean:
	rm -f *.o cs cc