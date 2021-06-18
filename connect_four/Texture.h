#pragma once
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include <string>
using namespace std;

class Texture {
private:
	SDL_Texture* texture = nullptr;
	SDL_Renderer* renderer = nullptr;
	uint w = 0;
	uint h = 0;
public:
	Texture(SDL_Renderer* r) : renderer(r) {};
	Texture(SDL_Renderer* r, string filename) : renderer(r) { load(filename); };
	~Texture() { Free(); };
	void Free();

	int getW() const { return w; };
	int getH() const { return h; };
	SDL_Texture* getTexture() const { return texture; };

	void load(string filename);
	void render(const SDL_Rect& rect, SDL_RendererFlip flip = SDL_FLIP_NONE) const;
};