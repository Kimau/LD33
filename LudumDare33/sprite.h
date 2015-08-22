#pragma once
#include <list>

int SetupSpritesFromFile(const char* filename);
SDL_Surface* LoadBMP(const char* filename);

std::list<std::list<SDL_Point>> ExtractHouses(uint16_t* pixs, int w, int h);
