CC=g++
CFLAGS=-g -I.
DEPS = Socket.h Chat.h SDLGame.h Texture.h
OBJ = Socket.o Chat.o SDLGame.o Texture.o
LIBS=-lpthread -lSDL2 -lSDL2_ttf -lSDL2_image

%.o: %.cc $(DEPS)
	$(CC) -g -c -o $@ $< $(CFLAGS)

all: cs cc

cs: $(OBJ) ChatServer.o
	g++ -o $@ $^ $(CFLAGS) $(LIBS)

cc: $(OBJ) ChatClient.o
	g++ -o $@ $^ $(CFLAGS) $(LIBS)


.PHONY: clean

clean:
	rm -f *.o cs cc

