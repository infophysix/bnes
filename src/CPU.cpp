#include "CPU.hpp"

#include "Bus.hpp"

#include <iomanip>
#include <iostream>

CPU::CPU(Bus& bus_ref)
	: bus { &bus_ref }
{
}

CPU::~CPU()
{
}

void CPU::step()
{
	current_cycles = 0;
	additional_cycles = 0;

	// Fetch
	uint8_t opcode = fetchByte();

	// Decode
	Info info = decodeOpcode(opcode);

	// Execute
	executeInstruction(info.instruction, info.addr_mode, info.num_cycles);

	// Tick
	uint8_t total_cycles = current_cycles + additional_cycles;
	bus->tick(total_cycles);
}

////////////////////
// Timing
////////////////////

void CPU::tick(uint8_t cycles)
{
	bus->tick(cycles);
}

////////////////////
// Interrupts
////////////////////

void CPU::handleInterrupt(Interrupt interrupt)
{
	switch (interrupt)
	{
	case Interrupt::NMI:
	{
		stackPush(highByte(PC));
		stackPush(lowByte(PC));
		setFlag(Flag::B, 1);
		stackPush(P);
		setFlag(Flag::B, 0);
		setFlag(Flag::I, 1);

		uint8_t PCL = read(0xFFFA);
		uint8_t PCH = read(0xFFFB);

		PC = buildAddress(PCH, PCL);
	}
	break;

	case Interrupt::RESET:
	{
		SP = 0xFD;
		P = 0x04;
		uint8_t PCL = read(0xFFFC);
		uint8_t PCH = read(0xFFFD);
		PC = buildAddress(PCH, PCL);
	}
	break;

	case Interrupt::IRQ:
	{
		if (getFlag(Flag::I) == 1)
			return;
		stackPush(highByte(PC));
		stackPush(lowByte(PC));
		stackPush(P);
		setFlag(Flag::I, 1);
		uint8_t PCL = read(0xFFFE);
		uint8_t PCH = read(0xFFFF);
		PC = buildAddress(PCH, PCL);
	}
	break;
	}
}

////////////////////
// Data Access
////////////////////

uint8_t CPU::read(uint16_t addr) const
{
	uint8_t data = bus->cpuRead(addr);

	return data;
}

void CPU::write(uint16_t addr, uint8_t data)
{
	bus->cpuWrite(addr, data);
}

////////////////////
// Helpers
////////////////////

uint8_t CPU::fetchByte()
{
	return read(PC++);
}

uint16_t CPU::fetchWord()
{
	uint8_t offset = fetchByte();
	uint8_t page = fetchByte();

	return buildAddress(page, offset);
}

inline uint16_t CPU::buildAddress(uint8_t high, uint8_t low) const
{
	uint16_t addr = (high << 8) | low;

	return addr;
}

inline uint8_t CPU::lowByte(uint16_t word)
{
	return word & 0x00FF;
}

inline uint8_t CPU::highByte(uint16_t word)
{
	return word >> 8;
}

void CPU::checkPageCross(uint16_t addr, int8_t s_offset)
{
	uint8_t page_before = highByte(addr);
	// address += s_offset;
	addr += s_offset;
	uint8_t page_after = highByte(addr);

	if (page_before != page_after)
		additional_cycles++;
}

////////////////////
// Stack
////////////////////

void CPU::stackPush(uint8_t data)
{
	write(0x0100 + SP, data);
	SP--;
}

uint8_t CPU::stackPop()
{
	SP++;
	uint8_t data = read(0x0100 + SP);
	return data;
}

////////////////////
// Flags
////////////////////

bool CPU::getFlag(Flag flag) const
{
	int bit_pos = static_cast<int>(flag);

	return P & bit_pos;
}

void CPU::setFlag(Flag flag, bool condition)
{
	int bit_pos = static_cast<int>(flag);

	if (condition == true)
		P |= bit_pos;
	else
		P &= ~bit_pos;
}

////////////////////
// Addressing Modes
////////////////////

uint16_t CPU::fetchOperandAddress(AddressingMode mode)
{
	switch (mode)
	{
	case AddressingMode::Absolute:
	{
		uint8_t PCL = fetchByte();
		uint8_t PCH = fetchByte();

		return buildAddress(PCH, PCL);
	}

	case AddressingMode::AbsoluteX:
	{
		uint16_t addr = fetchWord();

		return addr + X;
	}

	case AddressingMode::AbsoluteY:
	{
		uint16_t addr = fetchWord();
		checkPageCross(addr, Y);

		return addr + Y;
	}

	case AddressingMode::Immediate:
	{
		uint16_t addr = PC++;

		return addr;
	}

	// accounts for bug
	// http://www.6502.org/tutorials/6502opcodes.html#JMP
	case AddressingMode::Indirect:
	{
		uint8_t ptr_low = fetchByte();
		uint8_t ptr_high = fetchByte();
		uint16_t ptr = buildAddress(ptr_high, ptr_low);
		uint8_t addr_low = read(ptr);
		uint8_t addr_high {};
		if (ptr_low == 0xFF)
		{
			uint16_t temp = buildAddress(ptr_high, 0x00);
			addr_high = read(temp);
		} else
		{
			addr_high = read(ptr + 1);
		}
		uint16_t addr = buildAddress(addr_high, addr_low);

		return addr;
	}

	case AddressingMode::IndirectX:
	{
		uint8_t zp_addr = fetchByte() + X; // wraps around
		uint8_t offset = read(0x00FF & zp_addr);
		uint8_t page = read(0x00FF & (zp_addr + 1));
		uint16_t addr = buildAddress(page, offset);

		return addr;
	}

	case AddressingMode::IndirectY:
	{
		uint8_t zp_ptr = fetchByte();
		uint8_t offset = read(0x00FF & zp_ptr);
		uint8_t page = read(0x00FF & (zp_ptr + 1));
		uint16_t addr = buildAddress(page, offset);
		checkPageCross(addr, Y);

		return addr + Y;
	}

	case AddressingMode::Relative:
	{
		uint16_t rel_addr = PC++;

		return rel_addr;
	}

	case AddressingMode::ZeroPage:
	{
		uint8_t zp_addr = fetchByte();
		uint16_t addr = buildAddress(0x00, zp_addr);

		return addr;
	}

	case AddressingMode::ZeroPageX:
	{
		uint8_t zp_addr = fetchByte() + X; // wraps around
		uint16_t addr = buildAddress(0x00, zp_addr);

		return addr;
	}

	case AddressingMode::ZeroPageY:
	{
		uint8_t zp_addr = fetchByte() + Y; // wraps around
		uint16_t addr = buildAddress(0x00, zp_addr);

		return addr;
	}

	// TODO: temp fix for 'Accumulator' and 'Implied' not handled
	default:
	{
		return 0;
	}
	}
}

uint8_t CPU::fetchOperand(AddressingMode mode)
{
	uint16_t operand_addr = fetchOperandAddress(mode);
	uint8_t data = read(operand_addr);

	return data;
}

////////////////////
// Instructions
////////////////////

CPU::Info CPU::decodeOpcode(uint8_t opcode)
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

void CPU::branch(uint8_t offset, bool condition)
{
	int8_t s_offset = static_cast<int8_t>(offset);
	uint16_t new_addr = PC + s_offset;

	if (condition == true)
	{
		// cycle for branch taken
		additional_cycles++;
		// cycle for page crossed
		checkPageCross(PC, offset);

		PC = new_addr;
	}
}

bool CPU::checkOverflow(uint8_t A, uint8_t operand, uint16_t result)
{
	// Overflow occurs in two scenarios,
	// 1. pos + pos = neg
	// 2. neg + neg = pos

	int8_t s_A = static_cast<int8_t>(A);
	int8_t s_operand = static_cast<int8_t>(operand);
	int8_t s_result = static_cast<int8_t>(result);

	bool overflow = false;

	if (s_A >= 0 && s_operand >= 0 && s_result < 0)
		overflow = true;
	if (s_A < 0 && s_operand < 0 && s_result >= 0)
		overflow = true;

	return overflow;
}

void CPU::executeInstruction(
	Instruction instruction,
	AddressingMode mode,
	uint8_t cycles
)
{
	current_cycles = cycles;

	switch (instruction)
	{
	case Instruction::ADC:
	{
		uint8_t operand = fetchOperand(mode);
		uint16_t result = A + operand + getFlag(Flag::C);

		setFlag(Flag::V, checkOverflow(A, operand, result));
		setFlag(Flag::C, result > 255);
		setFlag(Flag::Z, static_cast<uint8_t>(result) == 0);
		setFlag(Flag::N, result & 0b10000000);

		A = result;
	}
	break;

	case Instruction::AND:
	{
		A &= fetchOperand(mode);

		setFlag(Flag::Z, A == 0);
		setFlag(Flag::N, A & 0b10000000);
	}
	break;

	case Instruction::ASL:
	{
		if (mode == AddressingMode::Accumulator)
		{
			setFlag(Flag::C, A & 0b10000000);
			A <<= 1;
			setFlag(Flag::N, A & 0b10000000);
			setFlag(Flag::Z, A == 0);
		} else
		{
			uint16_t addr = fetchOperandAddress(mode);
			uint8_t data = read(addr);

			setFlag(Flag::C, data & 0b10000000);
			data <<= 1;
			setFlag(Flag::N, data & 0b10000000);
			setFlag(Flag::Z, data == 0);

			write(addr, data);
		}
	}
	break;

	case Instruction::BCC:
	{
		uint8_t offset = fetchOperand(mode);

		branch(offset, getFlag(Flag::C) == 0);
	}
	break;

	case Instruction::BCS:
	{
		uint8_t offset = fetchOperand(mode);

		branch(offset, getFlag(Flag::C) == 1);
	}
	break;

	case Instruction::BEQ:
	{
		uint8_t offset = fetchOperand(mode);

		branch(offset, getFlag(Flag::Z) == 1);
	}
	break;

	case Instruction::BIT:
	{
		uint8_t operand = fetchOperand(mode);
		uint8_t result = operand & A;

		setFlag(Flag::N, operand & 0b10000000);
		setFlag(Flag::V, operand & 0b01000000);
		setFlag(Flag::Z, result == 0);
	}
	break;

	case Instruction::BMI:
	{
		uint8_t offset = fetchOperand(mode);

		branch(offset, getFlag(Flag::N) == 1);
	}
	break;

	case Instruction::BNE:
	{
		uint8_t offset = fetchOperand(mode);

		branch(offset, getFlag(Flag::Z) == 0);
	}
	break;

	case Instruction::BPL:
	{
		uint8_t offset = fetchOperand(mode);

		branch(offset, getFlag(Flag::N) == 0);
	}
	break;

	case Instruction::BRK:
	{
		stackPush(highByte(PC));
		stackPush(lowByte(PC));

		setFlag(Flag::B, 1);
		stackPush(P);
		setFlag(Flag::B, 0);

		uint8_t PCL = read(0xFFFE);
		uint8_t PCH = read(0xFFFF);

		PC = buildAddress(PCH, PCL);
	}
	break;

	case Instruction::BVC:
	{
		uint8_t offset = fetchOperand(mode);

		branch(offset, getFlag(Flag::V) == 0);
	}
	break;

	case Instruction::BVS:
	{
		uint8_t offset = fetchOperand(mode);

		branch(offset, getFlag(Flag::V) == 1);
	}
	break;

	case Instruction::CLC:
	{
		setFlag(Flag::C, 0);
	}
	break;

	case Instruction::CLD:
	{
		setFlag(Flag::D, 0);
	}
	break;

	case Instruction::CLI:
	{
		setFlag(Flag::I, 0);
	}
	break;

	case Instruction::CLV:
	{
		setFlag(Flag::V, 0);
	}
	break;

	case Instruction::CMP:
	{
		uint8_t operand = fetchOperand(mode);
		uint8_t result = A - operand;

		setFlag(Flag::Z, result == 0);
		setFlag(Flag::C, operand <= A);
		setFlag(Flag::N, result & 0b10000000);
	}
	break;

	case Instruction::CPX:
	{
		uint8_t operand = fetchOperand(mode);
		uint8_t result = X - operand;

		setFlag(Flag::Z, operand == X);
		setFlag(Flag::C, operand <= X);
		setFlag(Flag::N, result & 0b10000000);
	}
	break;

	case Instruction::CPY:
	{
		uint8_t data = fetchOperand(mode);
		uint8_t result = Y - data;

		setFlag(Flag::Z, data == Y);
		setFlag(Flag::C, data <= Y);
		setFlag(Flag::N, result & 0b10000000);
	}
	break;

	case Instruction::DEC:
	{
		uint16_t addr = fetchOperandAddress(mode);
		uint8_t data = read(addr);
		uint8_t result = data - 1;
		write(addr, result);

		setFlag(Flag::Z, result == 0);
		setFlag(Flag::N, result & 0b10000000);
	}
	break;

	case Instruction::DEX:
	{
		X--;

		setFlag(Flag::Z, X == 0);
		setFlag(Flag::N, X & 0b10000000);
	}
	break;

	case Instruction::DEY:
	{
		Y--;

		setFlag(Flag::Z, Y == 0);
		setFlag(Flag::N, Y & 0b10000000);
	}
	break;

	case Instruction::EOR:
	{
		A ^= fetchOperand(mode);

		setFlag(Flag::Z, A == 0);
		setFlag(Flag::N, A & 0b10000000);
	}
	break;

	case Instruction::INC:
	{
		uint16_t addr = fetchOperandAddress(mode);
		uint8_t data = read(addr);
		uint8_t result = data + 1;

		write(addr, result);

		setFlag(Flag::Z, result == 0);
		setFlag(Flag::N, result & 0b10000000);
	}
	break;

	case Instruction::INX:
	{
		X++;

		setFlag(Flag::Z, X == 0);
		setFlag(Flag::N, X & 0b10000000);
	}
	break;

	case Instruction::INY:
	{
		Y++;

		setFlag(Flag::Z, Y == 0);
		setFlag(Flag::N, Y & 0b10000000);
	}
	break;

	case Instruction::JMP:
	{
		PC = fetchOperandAddress(mode);
	}
	break;

	case Instruction::JSR:
	{
		uint16_t addr = fetchOperandAddress(mode);

		stackPush(highByte(PC));
		stackPush(lowByte(PC));

		PC = addr;
	}
	break;

	case Instruction::LDA:
	{
		A = fetchOperand(mode);

		setFlag(Flag::Z, A == 0);
		setFlag(Flag::N, A & 0b10000000);
	}
	break;

	case Instruction::LDX:
	{
		X = fetchOperand(mode);

		setFlag(Flag::Z, X == 0);
		setFlag(Flag::N, X & 0b10000000);
	}
	break;

	case Instruction::LDY:
	{
		Y = fetchOperand(mode);

		setFlag(Flag::Z, Y == 0);
		setFlag(Flag::N, Y & 0b10000000);
	}
	break;

	case Instruction::LSR:
	{
		uint8_t result {};

		if (mode == AddressingMode::Accumulator)
		{
			// carry is set to first bit of input
			setFlag(Flag::C, A & 0b00000001);
			A >>= 1;
			result = A;
		} else
		{
			uint16_t addr = fetchOperandAddress(mode);
			uint8_t data = read(addr);
			setFlag(Flag::C, data & 0b00000001);
			result = data >>= 1;
			write(addr, result);
		}

		setFlag(Flag::N, 0);
		setFlag(Flag::Z, result == 0);
	}
	break;

	case Instruction::NOP:
	{
	}
	break;

	case Instruction::ORA:
	{
		A |= fetchOperand(mode);

		setFlag(Flag::Z, A == 0);
		setFlag(Flag::N, A & 0b10000000);
	}
	break;

	case Instruction::PHA:
	{
		stackPush(A);
	}
	break;

	case Instruction::PHP:
	{
		setFlag(Flag::B, 1);
		stackPush(P);
		setFlag(Flag::B, 0);
	}
	break;

	case Instruction::PLA:
	{
		A = stackPop();

		setFlag(Flag::Z, A == 0);
		setFlag(Flag::N, A & 0b10000000);
	}
	break;

	case Instruction::PLP:
	{
		P = stackPop();
		setFlag(Flag::B, 0);
		setFlag(Flag::U, 1); // TODO: this needs to be always on
	}
	break;

	case Instruction::ROL:
	{
		uint8_t input {};
		uint8_t result {};

		if (mode == AddressingMode::Accumulator)
		{
			input = A;
			A = std::rotl(A, 1);
			result = A;
		} else
		{
			uint16_t addr = fetchOperandAddress(mode);
			uint8_t data = read(addr);
			input = data;
			result = std::rotl(data, 1);
			write(addr, result);
		}

		setFlag(Flag::C, input & 0b10000000);
		setFlag(Flag::N, input & 0b01000000);
		setFlag(Flag::Z, result == 0);
	}
	break;

	case Instruction::ROR:
	{
		uint8_t input {};
		uint8_t result {};

		setFlag(Flag::N, getFlag(Flag::C));

		if (mode == AddressingMode::Accumulator)
		{
			input = A;
			A >>= 1;
			if (getFlag(Flag::C) == 1)
				A |= 0b10000000;
			else
				A &= 0b01111111;

			setFlag(Flag::C, input & 0b00000001);
			result = A;
		} else
		{
			uint16_t addr = fetchOperandAddress(mode);
			uint8_t data = read(addr);
			input = data;
			data >>= 1;
			if (getFlag(Flag::C) == 1)
				data |= 0b10000000;
			else
				data &= 0b01111111;

			setFlag(Flag::C, input & 0b00000001);
			result = data;
			write(addr, result);
		}

		setFlag(Flag::Z, result == 0);
	}
	break;

	case Instruction::RTI:
	{
		P = stackPop();
		setFlag(Flag::B, 0);
		setFlag(Flag::U, 1); // TODO: needs to be always on
		uint8_t PCL = stackPop();
		uint8_t PCH = stackPop();

		PC = buildAddress(PCH, PCL);
	}
	break;

	case Instruction::RTS:
	{
		uint8_t PCL = stackPop();
		uint8_t PCH = stackPop();

		PC = buildAddress(PCH, PCL);
	}
	break;

	case Instruction::SBC:
	{
		uint8_t operand = fetchOperand(mode);
		int16_t result =
			static_cast<int16_t>(A - operand - (1 - getFlag(Flag::C)));

		// https://www.doc.ic.ac.uk/~eedwards/compsys/arithmetic/index.html
		int8_t s_A = static_cast<int8_t>(A);
		int8_t s_M = static_cast<int8_t>(operand);
		bool overflow = false;

		if (s_A >= 0 && s_M < 0 && result < 0)
			overflow = true;
		if (s_A < 0 && s_M >= 0 && result >= 0)
			overflow = true;

		setFlag(Flag::V, overflow);
		// http://forum.6502.org/viewtopic.php?f=2&t=2944
		setFlag(Flag::C, A >= operand);
		setFlag(Flag::Z, static_cast<uint8_t>(result) == 0);
		setFlag(Flag::N, static_cast<uint8_t>(result) & 0b10000000);

		A = static_cast<uint8_t>(result);
	}
	break;

	case Instruction::SEC:
	{
		setFlag(Flag::C, 1);
	}
	break;

	case Instruction::SED:
	{
		setFlag(Flag::D, 1);
	}
	break;

	case Instruction::SEI:
	{
		setFlag(Flag::I, 1);
	}
	break;

	case Instruction::STA:
	{
		uint16_t target_addr = fetchOperandAddress(mode);

		write(target_addr, A);
	}
	break;

	case Instruction::STX:
	{
		uint16_t target_addr = fetchOperandAddress(mode);

		write(target_addr, X);
	}
	break;

	case Instruction::STY:
	{
		uint16_t target_addr = fetchOperandAddress(mode);

		write(target_addr, Y);
	}
	break;

	case Instruction::TAX:
	{
		X = A;

		setFlag(Flag::Z, X == 0);
		setFlag(Flag::N, X & 0b10000000);
	}
	break;

	case Instruction::TAY:
	{
		Y = A;

		setFlag(Flag::Z, Y == 0);
		setFlag(Flag::N, Y & 0b10000000);
	}
	break;

	case Instruction::TSX:
	{
		X = SP;

		setFlag(Flag::Z, X == 0);
		setFlag(Flag::N, X & 0b10000000);
	}
	break;

	case Instruction::TXA:
	{
		A = X;

		setFlag(Flag::Z, A == 0);
		setFlag(Flag::N, A & 0b10000000);
	}
	break;

	case Instruction::TXS:
	{
		SP = X;
	}
	break;

	case Instruction::TYA:
	{
		A = Y;

		setFlag(Flag::Z, A == 0);
		setFlag(Flag::N, A & 0b10000000);
	}
	break;
	}
}