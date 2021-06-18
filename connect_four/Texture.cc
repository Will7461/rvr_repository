#include "Texture.h"
#include <iostream>

using namespace std;

void Texture::Free() {
	SDL_DestroyTexture(texture);
	texture = nullptr;
	w = h = 0;
}

void Texture::load(string filename) {
	SDL_Surface* tempSurface = IMG_Load(filename.c_str());
	if (tempSurface == nullptr) {
		std::cerr << "Error loading surface from " + filename + "\n";
		return;
	}
	Free();
	texture = SDL_CreateTextureFromSurface(renderer, tempSurface);
	if (texture == nullptr){
		std::cerr << "Error loading texture from " + filename + "\n";
		return;
	}

	w = tempSurface->w;
	h = tempSurface->h;

	SDL_FreeSurface(tempSurface);
}

void Texture::render(const SDL_Rect& destRect, SDL_RendererFlip flip) const {
	SDL_RenderCopyEx(renderer, texture, NULL, &destRect, 0, 0, flip);
}