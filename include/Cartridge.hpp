#pragma once

#include "Mapper.hpp"
#include "Mapper000.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

constexpr size_t PRG_BANK_SIZE = 16384; // 16 KB
constexpr size_t CHR_BANK_SIZE = 8192;  // 8 KB

////////////////////
// iNES Header
////////////////////

struct iNESHeader
{
	char preamble[4];
	uint8_t prg_rom_banks;
	uint8_t chr_rom_banks;

	struct
	{
		char mirroring   : 1;
		char battery     : 1;
		char trainer     : 1;
		char four_screen : 1;
		char mapper_low  : 4;
	} flags_6;

	struct
	{
		char reserved    : 4;
		char mapper_high : 4;
	} flags_7;

	uint8_t prg_ram_banks;
	char garbage[7];
};

////////////////////
// Cartridge
////////////////////

class Cartridge
{
public:

	Cartridge();
	~Cartridge();

	////////////////////
	// Initialization
	////////////////////

	void loadROM(const std::string& rom_file);

	////////////////////
	// Testing
	////////////////////

	void printHeader() const;
	void printROM() const;
	void logROM(const std::string& log_file) const;

	////////////////////
	// Data
	////////////////////

	uint8_t prg_banks;
	uint8_t chr_banks;

	std::vector<uint8_t> CHR_ROM;
	std::vector<uint8_t> PRG_ROM;

	////////////////////
	// Data access
	////////////////////

	uint8_t readPRG(uint16_t addr) const;
	uint8_t readCHR(uint16_t addr) const;

	////////////////////
	// Mirroring
	////////////////////

	enum class Mirroring
	{
		Horizontal,
		Vertical,
		FourScreen
	} mirroring;

private:

	////////////////////
	// Header
	////////////////////

	iNESHeader header;

	////////////////////
	// Battery
	////////////////////

	bool battery_backed;

	////////////////////
	// Trainer
	////////////////////

	bool trainer_present;

	////////////////////
	// PRG RAM
	////////////////////

	uint8_t prg_ram_banks;

	////////////////////
	// Mapper
	////////////////////

	uint8_t mapper_id;

	std::unique_ptr<Mapper> mapper;
};