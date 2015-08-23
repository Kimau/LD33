#pragma once
#include <list>

int SetupSpritesFromFile(const char* filename);
SDL_Surface* LoadBMP(const char* filename);
void ClearBackground(uint16_t* pixs, int w, int h);

struct HousePoint {
	int x, y;
	bool isDoor;
};

std::list<std::list<HousePoint>> ExtractHouses(uint16_t* pixs, int w, int h);


struct Vec2 {
	float x, y;

	Vec2(float _x, float _y) : x{ _x }, y{ _y } {}
	Vec2(int _x, int _y) {
		x = (float)_x;
		y = (float)_y;
	}
};

Vec2 Normalise(const Vec2& v);
Vec2 operator+(const Vec2& a, const Vec2& b);
Vec2 operator-(const Vec2& a, const Vec2& b);