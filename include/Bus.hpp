#pragma once

#include "Cartridge.hpp"
#include "CPU.hpp"
#include "PPU.hpp"

#include <array>
#include <cstdlib>

class Bus
{
public:

	Bus();
	~Bus();

	////////////////////
	// Devices
	////////////////////

	CPU *cpu;

	void connectCartridge(Cartridge&);
	void connectCPU(CPU&);
	void connectPPU(PPU&);

	////////////////////
	// Timing
	////////////////////

	uint32_t cpu_cycles {};

	void tick(uint8_t cycles);

	////////////////////
	// Data access
	////////////////////

	uint8_t cpuRead(uint16_t addr) const;
	void cpuWrite(uint16_t addr, uint8_t data);

	uint8_t ppuRead(uint16_t addr) const;
	void ppuWrite(uint16_t addr, uint8_t data);

	////////////////////
	// Cartridge
	////////////////////

	Cartridge *cartridge;

private:

	////////////////////
	// CPU
	////////////////////

	std::array<uint8_t, 2048> RAM {};

	////////////////////
	// PPU
	////////////////////

	PPU *ppu;

	std::array<uint8_t, 2048> VRAM {};
};