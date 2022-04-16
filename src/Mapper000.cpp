#include "Mapper000.hpp"

#include "Cartridge.hpp"

Mapper000::Mapper000(Cartridge *cart_ref)
	: Mapper { cart_ref }
{
}

Mapper000::~Mapper000()
{
}

////////////////////
// Data access
////////////////////

uint8_t Mapper000::readPRG(uint16_t addr) const
{
	// if PRGROM is 16KB
	//     CPU Address Bus          PRG ROM
	//     0x8000 -> 0xBFFF: Map    0x0000 -> 0x3FFF
	//     0xC000 -> 0xFFFF: Mirror 0x0000 -> 0x3FFF
	// if PRGROM is 32KB
	//     CPU Address Bus          PRG ROM
	//     0x8000 -> 0xFFFF: Map    0x0000 -> 0x7FFF

	switch (addr)
	{
	case 0x8000 ... 0xFFFF:
		if (cartridge->prg_banks == 1)
			return cartridge->PRG_ROM[addr & 0x3FFF];
		if (cartridge->prg_banks == 2)
			return cartridge->PRG_ROM[addr & 0x7FFF];
		break;

	default:
		return 0;
	}
}

uint8_t Mapper000::readCHR(uint16_t addr) const
{
	return cartridge->CHR_ROM[addr];
}

// Mapper 000 (NROM) does not write anything