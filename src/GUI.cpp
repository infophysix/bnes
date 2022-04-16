#include "GUI.hpp"

GUI::GUI()
{
	// TODO: SDL_GetError

	SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO);

	window = SDL_CreateWindow(
		"bnes",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		WIDTH * SCALE,
		HEIGHT * SCALE,
		SDL_WINDOW_SHOWN
	);

	if (window == nullptr)
		throw std::runtime_error("Error creating SDL_Window\n");

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);

	if (renderer == nullptr)
		throw std::runtime_error("Error creating SDL_Renderer\n");

	texture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		WIDTH,
		HEIGHT
	);

	if (renderer == nullptr)
		throw std::runtime_error("Error creating SDL_Texture\n");

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
}

GUI::~GUI()
{
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void GUI::renderFrame(uint32_t buffer[HEIGHT][WIDTH])
{
	std::array<uint32_t, HEIGHT * WIDTH> pixels {};

	for (size_t Y {}; Y < HEIGHT; ++Y)
		for (size_t X {}; X < WIDTH; ++X)
			pixels[Y * WIDTH + X] = buffer[Y][X];

	SDL_RenderClear(renderer);
	SDL_UpdateTexture(texture, nullptr, pixels.data(), WIDTH * 4);
	SDL_RenderCopy(renderer, texture, nullptr, nullptr);
	SDL_RenderPresent(renderer);
}