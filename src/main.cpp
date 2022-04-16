#include "Bus.hpp"
#include "Cartridge.hpp"
#include "CPU.hpp"
#include "PPU.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

// #define CPU_ONLY
// #define LOGGING

#ifdef LOGGING
#include "Logger.hpp"
#endif

#ifndef CPU_ONLY
#include "GUI.hpp"
#endif

int main(int argc, char **argv)
{
#ifdef LOGGING

	if (argc != 3)
		throw std::runtime_error("Usage: <ROM> <log file>\n");

	const std::string in_file = argv[1];
	const std::string out_file = argv[2];

#endif // LOGGING

#ifndef LOGGING

	if (argc != 2)
		throw std::runtime_error("Usage: <ROM>\n");

	const std::string in_file = argv[1];

#endif // !LOGGING

	////////////////////
	// Initialization
	////////////////////

	// Cartridge
	Cartridge cartridge;
	cartridge.loadROM(in_file);

	// Bus
	Bus bus;
	bus.connectCartridge(cartridge);

	// CPU
	CPU cpu { bus };
	bus.connectCPU(cpu);

	// PPU
	PPU ppu { bus };
	bus.connectPPU(ppu);

	////////////////////
	// Logging
	////////////////////

#ifdef LOGGING

	Logger logger { out_file, bus, cpu, ppu };

	cartridge.printHeader();
	cartridge.printROM();
	cartridge.logROM(out_file);

#endif

	////////////////////
	// Inititialize
	////////////////////

	cpu.handleInterrupt(CPU::Interrupt::RESET);

	////////////////////
	// Main
	////////////////////

#ifdef LOGGING

	std::ofstream ofs { out_file };
	std::cout << "Main Loop\n";
	std::cout << "-------------------------\n";

#endif // LOGGING

#ifdef CPU_ONLY

	for (size_t i = 0; i < 80000; ++i)
	{
#ifdef LOGGING
		logger.logLine();
#endif

		cpu.step();
	}
#endif // CPU_ONLY

#ifndef CPU_ONLY

	GUI gui {};

	bool running = true;
	while (running)
	{
// Logging
#ifdef LOGGING
		logger.logLine();
#endif

		cpu.step();

		if (ppu.update_screen == true)
		{
			ppu.updateBuffer();
			gui.renderFrame(ppu.buffer);

			ppu.update_screen = false;
		}

		while (SDL_PollEvent(&gui.event))
			if (gui.event.type == SDL_QUIT)
				running = false;
	}

#endif

#ifdef LOGGING
	ofs.close();
#endif
}