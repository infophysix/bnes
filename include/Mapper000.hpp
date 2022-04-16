#pragma once

#include "Mapper.hpp"

#include <iostream>

class Cartridge;

class Mapper000 : public Mapper
{
public:

	Mapper000(Cartridge *);
	~Mapper000();

	////////////////////
	// Data access
	////////////////////

	uint8_t readPRG(uint16_t addr) const;
	uint8_t readCHR(uint16_t addr) const;
};