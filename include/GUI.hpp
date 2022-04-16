#pragma once

#include <array>
#include <iostream>
#include <SDL.h>

constexpr size_t WIDTH { 256 };
constexpr size_t HEIGHT { 240 };
constexpr size_t SCALE { 3 };

class GUI
{
public:

	GUI();
	~GUI();

	void renderFrame(uint32_t buffer[HEIGHT][WIDTH]);

	SDL_Event event;

private:

	SDL_Window *window { nullptr };
	SDL_Renderer *renderer { nullptr };
	SDL_Texture *texture { nullptr };
};