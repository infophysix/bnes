#include "Bus.hpp"

#include <iomanip>
#include <iostream>

Bus::Bus()
{
}

Bus::~Bus()
{
}

////////////////////
// Devices
////////////////////

void Bus::connectCartridge(Cartridge& cart_ref)
{
	cartridge = &cart_ref;
}

void Bus::connectCPU(CPU& cpu_ref)
{
	cpu = &cpu_ref;
}

void Bus::connectPPU(PPU& ppu_ref)
{
	ppu = &ppu_ref;
}

////////////////////
// Timing
////////////////////

void Bus::tick(uint8_t cycles)
{
	cpu_cycles += cycles;
	ppu->step(cycles * 3);
}

////////////////////
// Data access
////////////////////

uint8_t Bus::cpuRead(uint16_t addr) const
{
	uint8_t data {};

	switch (addr)
	{
	// RAM
	case 0x0000 ... 0x1FFF:
		return RAM[addr % 0x2000];

	// PPU Registers
	case 0x2000 ... 0x3FFF:
		data = ppu->readRegister(addr % 8, false);
		break;

	// PRG ROM
	case 0x4018 ... 0xFFFF:
		return cartridge->readPRG(addr);

	default:
		return 0;
	}

	return data;
}

void Bus::cpuWrite(uint16_t addr, uint8_t data)
{
	switch (addr)
	{
	case 0x0000 ... 0x1FFF:
		RAM[addr % 0x2000] = data;
		break; // RAM
	case 0x2000 ... 0x3FFF:
		ppu->writeRegister(addr % 8, data);
		break; // PPU Registers
	}
}

uint8_t Bus::ppuRead(uint16_t addr) const
{
	switch (addr)
	{
	// Pattern Tables (CHR ROM)
	case 0x0000 ... 0x1FFF:
		return cartridge->readCHR(addr);

	// Nametables (VRAM)
	case 0x2000 ... 0x3EFF:
	{
		addr %= 0x2000;

		switch (cartridge->mirroring)
		{
		case Cartridge::Mirroring::Horizontal:
		{
			if (addr >= 0x0000 && addr <= 0x03FF)
				return ppu->nametable_0[addr % 0x0400];

			if (addr >= 0x0400 && addr <= 0x07FF)
				return ppu->nametable_1[addr % 0x0400];

			if (addr >= 0x0800 && addr <= 0x1BFF)
				return ppu->nametable_2[addr % 0x0400];

			if (addr >= 0x1C00 && addr <= 0x1FFF)
				return ppu->nametable_3[addr % 0x0400];
		}
		break;

		case Cartridge::Mirroring::Vertical:
		{
			throw std::runtime_error(
				"Error: vertical mirroring not implemented\n"
			);
			return 1;
		}
		break;

		default:
			return ppu->nametable_0[addr & 0x1000];
		}
	}
	break;

	// Color Palettes
	// $3F00 - $3F1F : palette indexes
	// $3F20 - $3FFF : mirrors above
	case 0x3F00 ... 0x3FFF:
	{
		uint8_t index = static_cast<uint8_t>(addr % 0x3F20);
		return index;
	}

	// Mirrors $0000-$3FFF
	case 0x4000 ... 0xFFFF:
		return ppuRead(addr % 0x4000);

	default:
		return 0;
	}
}

void Bus::ppuWrite(uint16_t addr, uint8_t data)
{
	switch (addr)
	{
	// Cannot write to $0000 - $2000 (CHR ROM)?
	case 0x0000 ... 0x1FFF:
		break;

	// Nametables (VRAM)
	case 0x2000 ... 0x3EFF:
	{
		addr %= 0x2000;

		switch (cartridge->mirroring)
		{
		case Cartridge::Mirroring::Horizontal:
		{
			if (addr >= 0x0000 && addr <= 0x03FF)
			{
				ppu->nametable_0[addr] = data;
			}

			if (addr >= 0x0400 && addr <= 0x07FF)
				ppu->nametable_1[addr % 0x0400] = data;

			if (addr >= 0x0800 && addr <= 0x1BFF)
				ppu->nametable_2[addr % 0x0800] = data;

			if (addr >= 0x1C00 && addr <= 0x1FFF)
				ppu->nametable_3[addr % 0x1C00] = data;
		}
		break;

		case Cartridge::Mirroring::Vertical:
		{
			throw std::runtime_error(
				"Error: vertical mirroring not implemented\n"
			);
		}
		break;

		default:
			ppu->nametable_0[addr & 0x1000] = data;
		}
	}
	break;

	// Color Palettes
	// $3F00 - $3F1F : palette indexes
	// $3F20 - $3FFF : mirrors above
	case 0x3F00 ... 0x3FFF:
		ppu->vram_palettes[addr - 0x3F00] = data;
		break;

	// Mirrors $0000-$3FFF
	case 0x4000 ... 0xFFFF:
		ppuWrite(addr % 0x4000, data);
		break;
	}
}