#include "Logger.hpp"

Logger::Logger(const std::string& log_file, Bus& _bus, CPU& _cpu, PPU& _ppu)
	: ofs { log_file }
	, bus { &_bus }
	, cpu { &_cpu }
	, ppu { &_ppu }
{
}

Logger::~Logger()
{
}

void Logger::logLine()
{
	uint16_t PC = cpu->PC;
	uint8_t opcode = cpuRead(PC);
	Info info = decodeOpcode(opcode);

	logPC(PC);
	logOpcode(opcode);
	logOperands(info, PC);
	logMnemonic(info.mnemonic);
	logAddressingMode(info, PC);
	logRegisters();
	logCycles(info);
	ofs << '\n';
}

uint8_t Logger::cpuRead(uint16_t addr)
{
	uint8_t data {};

	if (addr >= 0x2000 && addr <= 0x3FFF)
		data = ppu->readRegister(addr % 8, true);
	else
		data = cpu->read(addr);

	return data;
}

////////////////////
// Logging Functions
////////////////////

void Logger::logPC(uint16_t PC)
{
	ofs << std::setw(4) << std::setfill('0') << std::uppercase << std::hex;
	ofs << static_cast<int>(PC);
	ofs << "  ";
}

void Logger::logOpcode(uint8_t opcode)
{
	ofs << std::setw(2) << std::setfill('0') << std::hex;
	ofs << static_cast<int>(opcode);
	ofs << ' ';
}

void Logger::logMnemonic(const std::string& mnemonic)
{
	ofs << mnemonic;
	ofs << ' ';
}

void Logger::logOperands(const Info& info, uint16_t PC)
{
	switch (info.addr_mode)
	{
	case AddressingMode::Absolute:
	case AddressingMode::AbsoluteX:
	case AddressingMode::AbsoluteY:
	case AddressingMode::Indirect:
	{
		uint8_t op1 = cpuRead(PC + 1);
		uint8_t op2 = cpuRead(PC + 2);

		ofs << std::setw(2) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(op1);
		ofs << ' ';
		ofs << std::setw(2) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(op2);
		ofs << "  ";
	}
	break;

	case AddressingMode::Relative:
	{
		uint8_t offset = cpuRead(PC + 1);

		ofs << std::setw(2) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(offset);
		ofs << "     ";
	}
	break;

	case AddressingMode::IndirectX:
	case AddressingMode::IndirectY:
	case AddressingMode::Immediate:
	case AddressingMode::ZeroPage:
	case AddressingMode::ZeroPageX:
	case AddressingMode::ZeroPageY:
	{
		uint8_t op1 = cpuRead(PC + 1);

		ofs << std::setw(2) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(op1);
		ofs << "     ";
	}
	break;

	default:
	{
		ofs << "       ";
	}
	}
}

void Logger::logAddressingMode(const Info& info, uint16_t PC)
{
	switch (info.addr_mode)
	{
	case AddressingMode::Absolute:
	{
		uint8_t lo = cpuRead(PC + 1);
		uint8_t hi = cpuRead(PC + 2);

		ofs << '$';
		ofs << std::setw(2) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(hi);
		ofs << std::setw(2) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(lo);

		switch (info.instruction)
		{
		case Instruction::JMP:
		case Instruction::JSR:
		{
			ofs << std::setfill(' ') << std::setw(25);
		}
		break;

		default:
		{
			uint16_t abs_addr = (hi << 8) | lo;
			uint8_t val_at_addr = cpuRead(abs_addr);
			ofs << " = ";
			ofs << std::setw(2) << std::setfill('0') << std::hex;
			ofs << static_cast<int>(val_at_addr);
			ofs << std::setfill(' ') << std::setw(20);
		}
		break;
		}
	}
	break;

	case AddressingMode::AbsoluteX:
	{
		uint8_t lo = cpuRead(PC + 1);
		uint8_t hi = cpuRead(PC + 2);
		uint16_t addr = (hi << 8) | lo;
		uint16_t abs_addr = addr + cpu->X;
		uint8_t data = cpuRead(abs_addr);

		ofs << '$';
		ofs << std::setw(2) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(hi);
		ofs << std::setw(2) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(lo);
		ofs << ",X @ ";
		ofs << std::setw(4) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(abs_addr);
		ofs << " = ";
		ofs << std::setw(2) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(data);
		ofs << std::setfill(' ') << std::setw(11);
	}
	break;

	case AddressingMode::AbsoluteY:
	{
		uint8_t lo = cpuRead(PC + 1);
		uint8_t hi = cpuRead(PC + 2);
		uint16_t addr = (hi << 8) | lo;
		uint16_t abs_addr = addr + cpu->Y;
		uint8_t data = cpuRead(abs_addr);

		ofs << '$';
		ofs << std::setw(2) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(hi);
		ofs << std::setw(2) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(lo);
		ofs << ",Y @ ";
		ofs << std::setw(4) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(abs_addr);
		ofs << " = ";
		ofs << std::setw(2) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(data);
		ofs << std::setfill(' ') << std::setw(11);
	}
	break;

	case AddressingMode::Accumulator:
	{
		ofs << 'A';
		ofs << std::setfill(' ') << std::setw(29);
	}
	break;

	case AddressingMode::Indirect:
	{
		uint8_t ptr_lo = cpuRead(PC + 1);
		uint8_t ptr_hi = cpuRead(PC + 2);
		uint16_t ptr = (ptr_hi << 8) | ptr_lo;

		uint8_t addr_lo = cpuRead(ptr);
		uint8_t addr_hi {};

		if (ptr_lo == 0xFF)
		{
			uint16_t tmp = (ptr_hi << 8) | 0x00;
			addr_hi = cpuRead(tmp);
		} else
		{
			addr_hi = cpuRead(ptr + 1);
		}
		uint16_t addr = (addr_hi << 8) | addr_lo;

		ofs << "($";
		ofs << std::setw(2) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(ptr_hi);
		ofs << std::setw(2) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(ptr_lo);
		ofs << ") = ";
		ofs << std::setw(4) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(addr);
		ofs << std::setfill(' ') << std::setw(16);
	}
	break;

	case AddressingMode::IndirectX:
	{
		uint8_t op1 = cpuRead(PC + 1);
		uint8_t zp_addr = op1 + cpu->X; // wraps around
		uint8_t offset = cpuRead(0x00FF & zp_addr);
		uint8_t page = cpuRead(0x00FF & (zp_addr + 1));
		uint16_t address = (page << 8) | offset;
		uint8_t data = cpuRead(address);

		ofs << "($";
		ofs << std::setw(2) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(op1);
		ofs << ",X) @ ";
		ofs << std::setw(2) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(zp_addr);
		ofs << " = ";
		ofs << std::setw(4) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(address);
		ofs << " = ";
		ofs << std::setw(2) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(data);

		ofs << std::setfill(' ') << std::setw(6);
	}
	break;

	case AddressingMode::IndirectY:
	{
		uint8_t zp_addr = cpuRead(PC + 1);
		uint8_t offset = cpuRead(0x00FF & zp_addr);
		uint8_t page = cpuRead(0x00FF & (zp_addr + 1));
		uint16_t addr = (page << 8) | offset;
		uint16_t addr2 = ((page << 8) | offset) + cpu->Y;
		uint8_t data = cpuRead(addr2);

		ofs << "($";
		ofs << std::setw(2) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(zp_addr);
		ofs << "),Y = ";
		ofs << std::setw(4) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(addr);
		ofs << " @ ";
		ofs << std::setw(4) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(addr2);
		ofs << " = ";
		ofs << std::setw(2) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(data);

		ofs << std::setfill(' ') << std::setw(4);
	}
	break;

	case AddressingMode::Immediate:
	{
		uint8_t val = cpuRead(PC + 1);

		ofs << "#$";
		ofs << std::setw(2) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(val);
		ofs << std::setfill(' ') << std::setw(26);
	}
	break;

	case AddressingMode::Relative:
	{
		int8_t s_offset = static_cast<int8_t>(cpuRead(PC + 1));
		// + 2 to skip opcode and operand
		uint16_t rel_addr = PC + 2 + s_offset;

		ofs << '$';
		ofs << std::setw(4) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(rel_addr);
		ofs << std::setfill(' ') << std::setw(25);
	}
	break;

	case AddressingMode::ZeroPage:
	{
		uint8_t zp_addr = cpuRead(PC + 1);
		uint16_t addr = 0x0000 | zp_addr;
		uint8_t val_at_addr = cpuRead(addr);

		ofs << '$';
		ofs << std::setw(2) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(zp_addr);
		ofs << " = ";
		ofs << std::setw(2) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(val_at_addr);
		ofs << std::setfill(' ') << std::setw(22);
	}
	break;

	case AddressingMode::ZeroPageX:
	{
		uint8_t zp_addr = cpuRead(PC + 1);
		uint8_t zp_addr_2 = zp_addr + cpu->X;
		uint16_t addr = (0x0000 | zp_addr_2);
		uint8_t data = cpuRead(addr);

		ofs << '$';
		ofs << std::setw(2) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(zp_addr);
		ofs << ",X @ ";
		ofs << std::setw(2) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(zp_addr_2);
		ofs << " = ";
		ofs << std::setw(2) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(data);
		ofs << std::setfill(' ') << std::setw(15);
	}
	break;

	case AddressingMode::ZeroPageY:
	{
		uint8_t zp_addr = cpuRead(PC + 1);
		uint8_t zp_addr_2 = zp_addr + cpu->Y;
		uint16_t addr = (0x0000 | zp_addr_2);
		uint8_t data = cpuRead(addr);

		ofs << '$';
		ofs << std::setw(2) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(zp_addr);
		ofs << ",Y @ ";
		ofs << std::setw(2) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(zp_addr_2);
		ofs << " = ";
		ofs << std::setw(2) << std::setfill('0') << std::hex;
		ofs << static_cast<int>(data);
		ofs << std::setfill(' ') << std::setw(15);
	}
	break;

	default:
	{
		ofs << std::setfill(' ') << std::setw(30);
	}
	}
}

void Logger::logRegisters()
{
	ofs << "A:" << std::setw(2) << std::setfill('0') << std::hex
		<< static_cast<int>(cpu->A) << ' ';
	ofs << "X:" << std::setw(2) << std::setfill('0') << std::hex
		<< static_cast<int>(cpu->X) << ' ';
	ofs << "Y:" << std::setw(2) << std::setfill('0') << std::hex
		<< static_cast<int>(cpu->Y) << ' ';
	ofs << "P:" << std::setw(2) << std::setfill('0') << std::hex
		<< static_cast<int>(cpu->P) << ' ';
	ofs << "SP:" << std::setw(2) << std::setfill('0') << std::hex
		<< static_cast<int>(cpu->SP) << ' ';
}

void Logger::logCycles(const Info& info)
{
	uint32_t cpu_total_cycles = bus->cpu_cycles;

	ofs << "PPU:";
	ofs << std::setw(3) << std::setfill(' ') << std::dec;
	ofs << static_cast<int>(ppu->scanlines);
	ofs << ',' << std::setw(3) << std::setfill(' ') << std::dec;
	ofs << static_cast<int>(ppu->cycles) << ' ';
	ofs << "CYC:" << static_cast<int>(cpu_total_cycles);
}

////////////////////
// Decode
////////////////////

Logger::Info Logger::decodeOpcode(uint8_t opcode) const
{
	switch(opcode)
	{

	//////////////////////////////////////////////////////////
	// mnemonic, instruction, addressing mode, # of cycles
	//////////////////////////////////////////////////////////

	case 0x00: return { "BRK", Instruction::BRK, AddressingMode::Implied, 7 };
	case 0x01: return { "ORA", Instruction::ORA, AddressingMode::IndirectX, 6 };
	// case 0x02:
	// case 0x03:
	// case 0x04:
	case 0x05: return { "ORA", Instruction::ORA, AddressingMode::ZeroPage, 3 };
	case 0x06: return { "ASL", Instruction::ASL, AddressingMode::ZeroPage, 5 };
	// case 0x07
	case 0x08: return { "PHP", Instruction::PHP, AddressingMode::Implied, 3 };
	case 0x09: return { "ORA", Instruction::ORA, AddressingMode::Immediate, 2 };
	case 0x0A: return { "ASL", Instruction::ASL, AddressingMode::Accumulator, 2 };
	// case 0x0B
	// case 0x0C
	case 0x0D: return { "ORA", Instruction::ORA, AddressingMode::Absolute, 4 };
	case 0x0E: return { "ASL", Instruction::ASL, AddressingMode::Absolute, 6 };
	// case 0x0F
	case 0x10: return { "BPL", Instruction::BPL, AddressingMode::Relative, 2 };
	case 0x11: return { "ORA", Instruction::ORA, AddressingMode::IndirectY, 5 };
	// case 0x12
	// case 0x13
	// case 0x14
	case 0x15: return { "ORA", Instruction::ORA, AddressingMode::ZeroPageX, 4 };
	case 0x16: return { "ASL", Instruction::ASL, AddressingMode::ZeroPageX, 6 };
	// case 0x07
	case 0x18: return { "CLC", Instruction::CLC, AddressingMode::Implied, 2 };
	case 0x19: return { "ORA", Instruction::ORA, AddressingMode::AbsoluteY, 4 };
	// case 0x1A
	// case 0x1B
	// case 0x1C
	case 0x1D: return { "ORA", Instruction::ORA, AddressingMode::AbsoluteX, 4 };
	case 0x1E: return { "ASL", Instruction::ASL, AddressingMode::AbsoluteX, 7 };
	case 0x20: return { "JSR", Instruction::JSR, AddressingMode::Absolute, 6 };
	case 0x21: return { "AND", Instruction::AND, AddressingMode::IndirectX, 6 };
	// case 0x22
	// case 0x23
	case 0x24: return { "BIT", Instruction::BIT, AddressingMode::ZeroPage, 3 };
	case 0x25: return { "AND", Instruction::AND, AddressingMode::ZeroPage, 3 };
	case 0x26: return { "ROL", Instruction::ROL, AddressingMode::ZeroPage, 5 };
	// case 0x27
	case 0x28: return { "PLP", Instruction::PLP, AddressingMode::Implied, 4 };
	case 0x29: return { "AND", Instruction::AND, AddressingMode::Immediate, 2 };
	case 0x2A: return { "ROL", Instruction::ROL, AddressingMode::Accumulator, 2 };
	// case 0x2B
	case 0x2C: return { "BIT", Instruction::BIT, AddressingMode::Absolute, 4 };
	case 0x2D: return { "AND", Instruction::AND, AddressingMode::Absolute, 4 };
	case 0x2E: return { "ROL", Instruction::ROL, AddressingMode::Absolute, 6 };
	case 0x30: return { "BMI", Instruction::BMI, AddressingMode::Relative, 2 };
	case 0x31: return { "AND", Instruction::AND, AddressingMode::IndirectY, 5 };
	// case 0x32
	// case 0x33
	// case 0x34
	case 0x35: return { "AND", Instruction::AND, AddressingMode::ZeroPageX, 4 };
	case 0x36: return { "ROL", Instruction::ROL, AddressingMode::ZeroPageX, 6 };
	// case 0x37
	case 0x38: return { "SEC", Instruction::SEC, AddressingMode::Implied, 2 };
	case 0x39: return { "AND", Instruction::AND, AddressingMode::AbsoluteY, 4 };
	// case 0x3A
	// case 0x3B
	// case 0x3C
	case 0x3D: return { "AND", Instruction::AND, AddressingMode::AbsoluteX, 4 };
	case 0x3E: return { "ROL", Instruction::ROL, AddressingMode::AbsoluteX, 7 };
	case 0x40: return { "RTI", Instruction::RTI, AddressingMode::Implied, 6 };
	case 0x41: return { "EOR", Instruction::EOR, AddressingMode::IndirectX, 6 };
	// case 0x42
	// case 0x43
	// case 0x44
	case 0x45: return { "EOR", Instruction::EOR, AddressingMode::ZeroPage, 3 };
	case 0x46: return { "LSR", Instruction::LSR, AddressingMode::ZeroPage, 5 };
	// case 0x47
	case 0x48: return { "PHA", Instruction::PHA, AddressingMode::Implied, 3 };
	case 0x49: return { "EOR", Instruction::EOR, AddressingMode::Immediate, 2 };
	case 0x4A: return { "LSR", Instruction::LSR, AddressingMode::Accumulator, 2 };
	// case 0x4B
	case 0x4C: return { "JMP", Instruction::JMP, AddressingMode::Absolute, 3 };
	case 0x4D: return { "EOR", Instruction::EOR, AddressingMode::Absolute, 4 };
	case 0x4E: return { "LSR", Instruction::LSR, AddressingMode::Absolute, 6 };
	case 0x50: return { "BVC", Instruction::BVC, AddressingMode::Relative, 2 };
	case 0x51: return { "EOR", Instruction::EOR, AddressingMode::IndirectY, 5 };
	// case 0x52
	// case 0x53
	// case 0x54
	case 0x55: return { "EOR", Instruction::EOR, AddressingMode::ZeroPageX, 4 };
	case 0x56: return { "LSR", Instruction::LSR, AddressingMode::ZeroPageX, 6 };
	// case 0x57
	case 0x58: return { "CLI", Instruction::CLI, AddressingMode::Implied, 2 };
	case 0x59: return { "EOR", Instruction::EOR, AddressingMode::AbsoluteY, 4 };
	// case 0x5A
	// case 0x5B
	// case 0x5C
	case 0x5D: return { "EOR", Instruction::EOR, AddressingMode::AbsoluteX, 4 };
	case 0x5E: return { "LSR", Instruction::LSR, AddressingMode::AbsoluteX, 7 };
	case 0x60: return { "RTS", Instruction::RTS, AddressingMode::Implied, 6 };
	case 0x61: return { "ADC", Instruction::ADC, AddressingMode::IndirectX, 6 };
	// case 0x62
	// case 0x63
	// case 0x64
	case 0x65: return { "ADC", Instruction::ADC, AddressingMode::ZeroPage, 3 };
	case 0x66: return { "ROR", Instruction::ROR, AddressingMode::ZeroPage, 5 };
	// case 0x67
	case 0x68: return { "PLA", Instruction::PLA, AddressingMode::Implied, 4 };
	case 0x69: return { "ADC", Instruction::ADC, AddressingMode::Immediate, 2 };
	case 0x6A: return { "ROR", Instruction::ROR, AddressingMode::Accumulator, 2 };
	// case 0x6B
	case 0x6C: return { "JMP", Instruction::JMP, AddressingMode::Indirect, 5 };
	case 0x6D: return { "ADC", Instruction::ADC, AddressingMode::Absolute, 4 };
	case 0x6E: return { "ROR", Instruction::ROR, AddressingMode::Absolute, 6 };
	case 0x70: return { "BVS", Instruction::BVS, AddressingMode::Relative, 2 };
	case 0x71: return { "ADC", Instruction::ADC, AddressingMode::IndirectY, 5 };
	// case 0x72
	// case 0x73
	// case 0x74
	case 0x75: return { "ADC", Instruction::ADC, AddressingMode::ZeroPageX, 4 };
	case 0x76: return { "ROR", Instruction::ROR, AddressingMode::ZeroPageX, 6 };
	// case 0x77
	case 0x78: return { "SEI", Instruction::SEI, AddressingMode::Implied, 2 };
	case 0x79: return { "ADC", Instruction::ADC, AddressingMode::AbsoluteY, 4 };
	// case 0x7A
	// case 0x7B
	// case 0x7C
	case 0x7D: return { "ADC", Instruction::ADC, AddressingMode::AbsoluteX, 4 };
	case 0x7E: return { "ROR", Instruction::ROR, AddressingMode::AbsoluteX, 7 };
	// case 0x80
	case 0x81: return { "STA", Instruction::STA, AddressingMode::IndirectX, 6 };
	// case 0x82
	// case 0x83
	case 0x84: return { "STY", Instruction::STY, AddressingMode::ZeroPage, 3 };
	case 0x85: return { "STA", Instruction::STA, AddressingMode::ZeroPage, 3 };
	case 0x86: return { "STX", Instruction::STX, AddressingMode::ZeroPage, 3 };
	// case 0x87
	case 0x88: return { "DEY", Instruction::DEY, AddressingMode::Implied, 2 };
	// case 0x89
	case 0x8A: return { "TXA", Instruction::TXA, AddressingMode::Implied, 2 };
	// case 0x8B
	case 0x8C: return { "STY", Instruction::STY, AddressingMode::Absolute, 4 };
	case 0x8D: return { "STA", Instruction::STA, AddressingMode::Absolute, 4 };
	case 0x8E: return { "STX", Instruction::STX, AddressingMode::Absolute, 4 };
	case 0x90: return { "BCC", Instruction::BCC, AddressingMode::Relative, 2 };
	case 0x91: return { "STA", Instruction::STA, AddressingMode::IndirectY, 6 };
	// case 0x92
	// case 0x93
	case 0x94: return { "STY", Instruction::STY, AddressingMode::ZeroPageX, 4 };
	case 0x95: return { "STA", Instruction::STA, AddressingMode::ZeroPageX, 4 };
	case 0x96: return { "STX", Instruction::STX, AddressingMode::ZeroPageY, 4 };
	// case 0x97
	case 0x98: return { "TYA", Instruction::TYA, AddressingMode::Implied, 2 };
	case 0x99: return { "STA", Instruction::STA, AddressingMode::AbsoluteY, 5 };
	case 0x9A: return { "TXS", Instruction::TXS, AddressingMode::Implied, 2 };
	// case 0x9B
	// case 0x9C
	case 0x9D: return { "STA", Instruction::STA, AddressingMode::AbsoluteX, 5 };
	// case 0x9E
	case 0xA0: return { "LDY", Instruction::LDY, AddressingMode::Immediate, 2 };
	case 0xA1: return { "LDA", Instruction::LDA, AddressingMode::IndirectX, 6 };
	case 0xA2: return { "LDX", Instruction::LDX, AddressingMode::Immediate, 2 };
	// case 0xA3
	case 0xA4: return { "LDY", Instruction::LDY, AddressingMode::ZeroPage, 3 };
	case 0xA5: return { "LDA", Instruction::LDA, AddressingMode::ZeroPage, 3 };
	case 0xA6: return { "LDX", Instruction::LDX, AddressingMode::ZeroPage, 3 };
	// case 0xA7
	case 0xA8: return { "TAY", Instruction::TAY, AddressingMode::Implied, 2 };
	case 0xA9: return { "LDA", Instruction::LDA, AddressingMode::Immediate, 2 };
	case 0xAA: return { "TAX", Instruction::TAX, AddressingMode::Implied, 2 };
	// case 0xAB
	case 0xAC: return { "LDY", Instruction::LDY, AddressingMode::Absolute, 4 };
	case 0xAD: return { "LDA", Instruction::LDA, AddressingMode::Absolute, 4 };
	case 0xAE: return { "LDX", Instruction::LDX, AddressingMode::Absolute, 4 };
	case 0xB0: return { "BCS", Instruction::BCS, AddressingMode::Relative, 2 };
	case 0xB1: return { "LDA", Instruction::LDA, AddressingMode::IndirectY, 5 };
	// case 0xB2
	// case 0xB3
	case 0xB4: return { "LDY", Instruction::LDY, AddressingMode::ZeroPageX, 4 };
	case 0xB5: return { "LDA", Instruction::LDA, AddressingMode::ZeroPageX, 4 };
	case 0xB6: return { "LDX", Instruction::LDX, AddressingMode::ZeroPageY, 4 };
	// case 0xB7
	case 0xB8: return { "CLV", Instruction::CLV, AddressingMode::Implied, 2 };
	case 0xB9: return { "LDA", Instruction::LDA, AddressingMode::AbsoluteY, 4 };
	case 0xBA: return { "TSX", Instruction::TSX, AddressingMode::Implied, 2 };
	// case 0xBB
	case 0xBC: return { "LDY", Instruction::LDY, AddressingMode::AbsoluteX, 4 };
	case 0xBD: return { "LDA", Instruction::LDA, AddressingMode::AbsoluteX, 4 };
	case 0xBE: return { "LDX", Instruction::LDX, AddressingMode::AbsoluteY, 4 };
	case 0xC0: return { "CPY", Instruction::CPY, AddressingMode::Immediate, 2 };
	case 0xC1: return { "CMP", Instruction::CMP, AddressingMode::IndirectX, 6 };
	// case 0xC2
	// case 0xC3
	case 0xC4: return { "CPY", Instruction::CPY, AddressingMode::ZeroPage, 3 };
	case 0xC5: return { "CMP", Instruction::CMP, AddressingMode::ZeroPage, 3 };
	case 0xC6: return { "DEC", Instruction::DEC, AddressingMode::ZeroPage, 5 };
	// case 0xC7
	case 0xC8: return { "INY", Instruction::INY, AddressingMode::Implied, 2 };
	case 0xC9: return { "CMP", Instruction::CMP, AddressingMode::Immediate, 2 };
	case 0xCA: return { "DEX", Instruction::DEX, AddressingMode::Implied, 2 };
	// case 0xCB
	case 0xCC: return { "CPY", Instruction::CPY, AddressingMode::Absolute, 4 };
	case 0xCD: return { "CMP", Instruction::CMP, AddressingMode::Absolute, 4 };
	case 0xCE: return { "DEC", Instruction::DEC, AddressingMode::Absolute, 6 };
	case 0xD0: return { "BNE", Instruction::BNE, AddressingMode::Relative, 2 };
	case 0xD1: return { "CMP", Instruction::CMP, AddressingMode::IndirectY, 5 };
	// case 0xD2
	// case 0xD3
	// case 0xD4
	case 0xD5: return { "CMP", Instruction::CMP, AddressingMode::ZeroPageX, 4 };
	case 0xD6: return { "DEC", Instruction::DEC, AddressingMode::ZeroPageX, 6 };
	// case 0xD7
	case 0xD8: return { "CLD", Instruction::CLD, AddressingMode::Implied, 2 };
	case 0xD9: return { "CMP", Instruction::CMP, AddressingMode::AbsoluteY, 4 };
	// case 0xDA
	// case 0xDB
	// case 0xDC
	case 0xDD: return { "CMP", Instruction::CMP, AddressingMode::AbsoluteX, 4 };
	case 0xDE: return { "DEC", Instruction::DEC, AddressingMode::AbsoluteX, 7 };
	case 0xE0: return { "CPX", Instruction::CPX, AddressingMode::Immediate, 2 };
	case 0xE1: return { "SBC", Instruction::SBC, AddressingMode::IndirectX, 6 };
	// case 0xE2
	// case 0xE3
	case 0xE4: return { "CPX", Instruction::CPX, AddressingMode::ZeroPage, 3 };
	case 0xE5: return { "SBC", Instruction::SBC, AddressingMode::ZeroPage, 3 };
	case 0xE6: return { "INC", Instruction::INC, AddressingMode::ZeroPage, 5 };
	// case 0xE7
	case 0xE8: return { "INX", Instruction::INX, AddressingMode::Implied, 2 };
	case 0xE9: return { "SBC", Instruction::SBC, AddressingMode::Immediate, 2 };
	case 0xEA: return { "NOP", Instruction::NOP, AddressingMode::Implied, 2 };
	// case 0xEB
	case 0xEC: return { "CPX", Instruction::CPX, AddressingMode::Absolute, 4 };
	case 0xED: return { "SBC", Instruction::SBC, AddressingMode::Absolute, 4 };
	case 0xEE: return { "INC", Instruction::INC, AddressingMode::Absolute, 6 };
	case 0xF0: return { "BEQ", Instruction::BEQ, AddressingMode::Relative, 2 };
	case 0xF1: return { "SBC", Instruction::SBC, AddressingMode::IndirectY, 5 };
	// case 0xF2
	// case 0xF3
	// case 0xF4
	case 0xF5: return { "SBC", Instruction::SBC, AddressingMode::ZeroPageX, 4 };
	case 0xF6: return { "INC", Instruction::INC, AddressingMode::ZeroPageX, 6 };
	// case 0xF7
	case 0xF8: return { "SED", Instruction::SED, AddressingMode::Implied, 2 };
	case 0xF9: return { "SBC", Instruction::SBC, AddressingMode::AbsoluteY, 4 };
	// case 0xFA
	// case 0xFB
	// case 0xFC
	case 0xFD: return { "SBC", Instruction::SBC, AddressingMode::AbsoluteX, 4 };
	case 0xFE: return { "INC", Instruction::INC, AddressingMode::AbsoluteX, 7 };
	default: return { "XXX", Instruction::NOP, AddressingMode::Implied, 2 };
	}
}