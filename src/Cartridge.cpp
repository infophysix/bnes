#include "Cartridge.hpp"

Cartridge::Cartridge()
{
}

Cartridge::~Cartridge()
{
}

////////////////////
// Initialization
////////////////////

void Cartridge::loadROM(const std::string& rom_file)
{
	std::ifstream ifs { rom_file, std::ios::binary };

	if (ifs.is_open() == true)
	{
		////////////////////
		// Read header
		////////////////////

		ifs.read(reinterpret_cast<char *>(&header), sizeof(iNESHeader));

		////////////////////
		// Resize ROMs
		////////////////////

		prg_banks = header.prg_rom_banks;
		chr_banks = header.chr_rom_banks;

		PRG_ROM.resize(prg_banks * PRG_BANK_SIZE);
		CHR_ROM.resize(chr_banks * CHR_BANK_SIZE);

		////////////////////
		// Mirroring type
		////////////////////

		if (header.flags_6.mirroring == 0)
			mirroring = Mirroring::Horizontal;
		if (header.flags_6.mirroring == 1)
			mirroring = Mirroring::Vertical;
		if (header.flags_6.four_screen == 1)
			mirroring = Mirroring::FourScreen;

		////////////////////
		// Battery
		////////////////////

		battery_backed = header.flags_6.battery;

		////////////////////
		// Trainer
		////////////////////

		trainer_present = header.flags_6.trainer;

		////////////////////
		// Mapper
		////////////////////

		uint8_t mapper_low = header.flags_6.mapper_low;
		uint8_t mapper_high = header.flags_7.mapper_high;

		mapper_id = (mapper_high << 4) & mapper_low;

		switch (mapper_id)
		{
		case 0:
			mapper = std::make_unique<Mapper000>(this);
			break;

		default:
			std::cerr << "Invalid mapper ID!\n";
		}

		////////////////////
		// PRG RAM
		////////////////////

		prg_ram_banks = header.prg_ram_banks;

		////////////////////
		// Read PRG/CHR data
		////////////////////

		ifs.read(reinterpret_cast<char *>(PRG_ROM.data()), PRG_ROM.size());
		ifs.read(reinterpret_cast<char *>(CHR_ROM.data()), CHR_ROM.size());

		ifs.close();
	} else
	{
		std::cerr << "Error loading ROM file!\n";
	}
}

////////////////////
// Testing
////////////////////

void Cartridge::printHeader() const
{
	std::cout << std::endl;
	std::cout << "iNES Header (" << sizeof(iNESHeader) << " bits)\n";
	std::cout << "-------------------------\n";
	std::cout << "Preamble:  " << sizeof(header.preamble) << " bits\n";
	std::cout << "PRG Banks: " << sizeof(header.prg_rom_banks) << " bits\n";
	std::cout << "CHR Banks: " << sizeof(header.chr_rom_banks) << " bits\n";
	std::cout << "Flags 6:   " << sizeof(header.flags_6) << " bits\n";
	std::cout << "Flags 7:   " << sizeof(header.flags_7) << " bits\n";
	std::cout << "PRG RAM:   " << sizeof(header.prg_ram_banks) << " bits\n";
	std::cout << "Garbage:   " << sizeof(header.garbage) << " bits\n\n";

	std::cout << std::endl;
	std::cout << "Header Information\n";
	std::cout << "-------------------------\n";
	std::cout << "Preamble:  " << header.preamble << '\n';
	std::cout << "PRG Banks: " << static_cast<int>(prg_banks) << '\n';
	std::cout << "CHR Banks: " << static_cast<int>(chr_banks) << '\n';
	std::cout << "Mirroring: ";
	if (mirroring == Mirroring::Horizontal)
		std::cout << "Horizontal\n";
	if (mirroring == Mirroring::Vertical)
		std::cout << "Vertical\n";
	if (mirroring == Mirroring::FourScreen)
		std::cout << "Four Screen\n";
	std::cout << "Battery:   " << std::boolalpha << battery_backed << '\n';
	std::cout << "Trainer:   " << std::boolalpha << trainer_present << '\n';
	std::cout << "Mapper:    " << static_cast<int>(mapper_id) << '\n';
	std::cout << "PRG RAM:   " << static_cast<int>(prg_ram_banks)
			  << " 8KB banks\n";
	std::cout << std::endl;
}

void Cartridge::printROM() const
{
	std::cout << std::endl;
	std::cout << "ROM Information\n";
	std::cout << "-------------------------\n";
	std::cout << "PRG ROM size: " << PRG_ROM.size() << '\n';
	std::cout << "CHR ROM size: " << CHR_ROM.size() << '\n';
	std::cout << std::endl;
}

void Cartridge::logROM(const std::string& log_file) const
{
	std::ofstream ofs { log_file };

	ofs << "PRG ROM: ";
	for (auto i : PRG_ROM)
		ofs << std::hex << static_cast<int>(i) << ' ';
	ofs << std::endl;

	ofs << "CHR ROM: ";
	for (auto i : CHR_ROM)
		ofs << std::hex << static_cast<int>(i) << ' ';
	ofs << std::endl;

	ofs.close();
}

////////////////////
// Data access
////////////////////

uint8_t Cartridge::readPRG(uint16_t addr) const
{
	return mapper->readPRG(addr);
}

uint8_t Cartridge::readCHR(uint16_t addr) const
{
	return mapper->readCHR(addr);
}