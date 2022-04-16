#pragma once

#include <iostream>

class Cartridge;

class Mapper
{
protected:

	Mapper(Cartridge *);

	////////////////////
	// Cartridge
	////////////////////

	Cartridge *cartridge;

public:

	virtual ~Mapper();

	////////////////////
	// Data access
	////////////////////

	virtual uint8_t readPRG(uint16_t addr) const = 0;
	virtual uint8_t readCHR(uint16_t addr) const = 0;
};